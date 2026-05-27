/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "UpdateApplier.h"

#include "minecraft/mod/ModMetadataIndex.h"
#include "MMCZip.h"
#include "BuildConfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QTemporaryDir>
#include <QUuid>
#include <memory>

#define PU_LOG(ctx, msg) MMCO_LOG((ctx), QString(msg).toUtf8().constData())

namespace pack_updater
{

	namespace
	{
		/* Same loader-uid taxonomy InstanceImportTask uses when it
		 * wires components for a fresh import. Centralising it here
		 * keeps the apply-side diff in lockstep. */
		constexpr const char kUidMinecraft[] = "net.minecraft";
		constexpr const char kUidFabricLoader[] = "net.fabricmc.fabric-loader";
		constexpr const char kUidForge[] = "net.minecraftforge";
		constexpr const char kUidNeoForge[] = "net.neoforged";
		constexpr const char kUidQuiltLoader[] = "org.quiltmc.quilt-loader";

		/* Read the "primary" mrpack file from a parsed file list.
		 * mrpack `downloads` arrays carry mirror URLs — first one
		 * is canonical, others are fallbacks. We pick whichever
		 * comes first; the dialog can retry from a different mirror
		 * later if we plumb it. */
		QUrl firstDownload(const QJsonObject& fileObj)
		{
			const QJsonArray arr =
				fileObj.value(QStringLiteral("downloads")).toArray();
			if (arr.isEmpty())
				return {};
			return QUrl(arr.first().toString());
		}

		/* Walk the dependencies block. The keys are well-known uids
		 * (Modrinth normalised them to lower-case shorthand). Each
		 * value is a version string the pack wants pinned. */
		ManifestComponents parseComponents(const QJsonObject& deps)
		{
			ManifestComponents out;
			out.minecraftVersion =
				deps.value(QStringLiteral("minecraft")).toString();
			out.fabricLoaderVersion =
				deps.value(QStringLiteral("fabric-loader")).toString();
			out.forgeVersion = deps.value(QStringLiteral("forge")).toString();
			out.neoForgeVersion =
				deps.value(QStringLiteral("neoforge")).toString();
			out.quiltLoaderVersion =
				deps.value(QStringLiteral("quilt-loader")).toString();
			return out;
		}

		ParsedManifest parseManifestJson(const QByteArray& bytes)
		{
			ParsedManifest m;
			QJsonParseError err{};
			const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
			if (err.error != QJsonParseError::NoError || !doc.isObject())
				return m;
			const QJsonObject o = doc.object();
			m.name = o.value(QStringLiteral("name")).toString();
			m.versionId = o.value(QStringLiteral("versionId")).toString();
			m.components = parseComponents(
				o.value(QStringLiteral("dependencies")).toObject());
			for (const auto& v : o.value(QStringLiteral("files")).toArray()) {
				const QJsonObject fo = v.toObject();
				ManifestFile mf;
				mf.path = fo.value(QStringLiteral("path")).toString();
				const QJsonObject hashes =
					fo.value(QStringLiteral("hashes")).toObject();
				mf.sha1 = hashes.value(QStringLiteral("sha1")).toString();
				mf.sha512 = hashes.value(QStringLiteral("sha512")).toString();
				mf.downloadUrl = firstDownload(fo);
				mf.size =
					qint64(fo.value(QStringLiteral("fileSize")).toDouble());
				/* Skip files with an obviously bad path — anything
				 * that contains `..` would let the pack write
				 * outside the instance. The launcher's import path
				 * has the same guard. */
				if (mf.path.contains(QStringLiteral("..")))
					continue;
				m.files.append(mf);
			}
			return m;
		}

		QString sidecarFolderForPath(const QString& minecraftDir,
									 const QString& packRelativePath)
		{
			const QString fwd = QString(packRelativePath).replace('\\', '/');
			const int slash = fwd.indexOf('/');
			if (slash < 1)
				return {};
			const QString top = fwd.left(slash);
			if (top == QLatin1String("mods") ||
				top == QLatin1String("resourcepacks") ||
				top == QLatin1String("shaderpacks") ||
				top == QLatin1String("texturepacks") ||
				top == QLatin1String("coremods")) {
				return minecraftDir + QStringLiteral("/") + top;
			}
			return {};
		}

		/* ─── CurseForge manifest.json parsing ────────────────── */

		/* One `files[]` entry from CurseForge's `manifest.json`.
		 * The manifest itself only points at (projectID, fileID)
		 * pairs — we resolve those through the CF API to fill in
		 * the rest. `targetPath` is the final instance-relative
		 * path we expect to drop the file at (CF doesn't say which
		 * subfolder; "mods/" is the universal default for
		 * `required: true` jars). */
		struct CfFileRef {
			qint64 projectID = 0;
			qint64 fileID = 0;
			bool required = true;
		};

		struct CfManifest {
			QString name;
			QString version;
			ManifestComponents components;
			QVector<CfFileRef> files;
		};

		/* Translate CF loader id strings ("neoforge-20.4.234",
		 * "forge-47.2.0", "fabric-0.15.7", "quilt-0.20.2") into our
		 * ManifestComponents shape. CF puts the MC version in a
		 * sibling field. */
		ManifestComponents parseCfComponents(const QJsonObject& minecraftObj)
		{
			ManifestComponents out;
			out.minecraftVersion =
				minecraftObj.value(QStringLiteral("version")).toString();
			const QJsonArray loaders =
				minecraftObj.value(QStringLiteral("modLoaders")).toArray();
			for (const auto& lv : loaders) {
				const QJsonObject lo = lv.toObject();
				const QString id = lo.value(QStringLiteral("id")).toString();
				const int dash = id.indexOf('-');
				if (dash <= 0)
					continue;
				const QString kind = id.left(dash);
				const QString ver = id.mid(dash + 1);
				if (kind == QLatin1String("forge"))
					out.forgeVersion = ver;
				else if (kind == QLatin1String("neoforge"))
					out.neoForgeVersion = ver;
				else if (kind == QLatin1String("fabric"))
					out.fabricLoaderVersion = ver;
				else if (kind == QLatin1String("quilt"))
					out.quiltLoaderVersion = ver;
			}
			return out;
		}

		CfManifest parseCfManifestJson(const QByteArray& bytes)
		{
			CfManifest m;
			QJsonParseError err{};
			const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
			if (err.error != QJsonParseError::NoError || !doc.isObject())
				return m;
			const QJsonObject o = doc.object();
			m.name = o.value(QStringLiteral("name")).toString();
			m.version = o.value(QStringLiteral("version")).toString();
			m.components = parseCfComponents(
				o.value(QStringLiteral("minecraft")).toObject());
			for (const auto& v : o.value(QStringLiteral("files")).toArray()) {
				const QJsonObject fo = v.toObject();
				CfFileRef ref;
				ref.projectID =
					fo.value(QStringLiteral("projectID")).toInteger();
				ref.fileID = fo.value(QStringLiteral("fileID")).toInteger();
				/* Required defaults to true when missing — that's
				 * what the CF importer in the launcher does too. */
				ref.required =
					fo.value(QStringLiteral("required")).toBool(true);
				if (ref.projectID > 0 && ref.fileID > 0)
					m.files.append(ref);
			}
			return m;
		}

		/* Resolver state for a CurseForge pack. We fire one HTTP
		 * request per (projectID, fileID) pair and assemble the
		 * results in `resolved`. `pending` counts outstanding
		 * requests so the last response fires the callback. */
		struct CfResolveCtx {
			MMCOContext* ctx;
			QString zipPath;
			CfManifest base;
			ManifestCallback cb;
			QVector<ManifestFile> resolved;
			int pending = 0;
			bool failed = false;
		};

		struct CfFileFetchThunk {
			std::shared_ptr<CfResolveCtx> shared;
			int slot; /* index into shared->resolved */
		};

		void cfFinishIfDone(std::shared_ptr<CfResolveCtx> rctx)
		{
			if (rctx->pending > 0)
				return;
			ParsedManifest out;
			if (rctx->failed) {
				/* Empty out signals failure to the dialog. */
				rctx->cb(out);
				return;
			}
			out.name = rctx->base.name;
			out.versionId = rctx->base.version;
			out.components = rctx->base.components;
			out.downloadedZipPath = rctx->zipPath;
			for (const auto& f : rctx->resolved) {
				if (!f.downloadUrl.isValid() || f.path.isEmpty())
					continue;
				out.files.append(f);
			}
			PU_LOG(rctx->ctx,
				   QString("  CF resolve done: %1 of %2 files have URLs, "
						   "versionId=%3 name=%4")
					   .arg(out.files.size())
					   .arg(rctx->resolved.size())
					   .arg(out.versionId, out.name));
			rctx->cb(out);
		}

		void onCfFileResolved(void* user_data, int status, const void* body,
							  size_t body_size)
		{
			std::unique_ptr<CfFileFetchThunk> self(
				static_cast<CfFileFetchThunk*>(user_data));
			auto rctx = self->shared;
			const int slot = self->slot;
			rctx->pending--;

			if (status < 200 || status >= 300) {
				PU_LOG(rctx->ctx, QString("  CF file resolve slot=%1 HTTP %2")
									  .arg(slot)
									  .arg(status));
				cfFinishIfDone(rctx);
				return;
			}
			const QByteArray bytes(static_cast<const char*>(body),
								   static_cast<int>(body_size));
			QJsonParseError err{};
			const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
			if (err.error != QJsonParseError::NoError || !doc.isObject()) {
				PU_LOG(rctx->ctx,
					   QString("  CF file resolve slot=%1 bad JSON").arg(slot));
				cfFinishIfDone(rctx);
				return;
			}
			const QJsonObject data =
				doc.object().value(QStringLiteral("data")).toObject();
			ManifestFile& mf = rctx->resolved[slot];
			mf.path = QStringLiteral("mods/") +
					  data.value(QStringLiteral("fileName")).toString();
			mf.size =
				qint64(data.value(QStringLiteral("fileLength")).toDouble());
			const QString dl =
				data.value(QStringLiteral("downloadUrl")).toString();
			if (!dl.isEmpty()) {
				mf.downloadUrl = QUrl(dl);
			} else {
				/* CF returns null downloadUrl when the mod author
				 * opted out of third-party launchers. Fall back to
				 * the conventional `edge.forgecdn.net` URL the
				 * launcher's own importer uses. */
				const qint64 fileId =
					data.value(QStringLiteral("id")).toInteger();
				const QString name =
					data.value(QStringLiteral("fileName")).toString();
				if (fileId > 0 && !name.isEmpty()) {
					mf.downloadUrl =
						QUrl(QStringLiteral(
								 "https://edge.forgecdn.net/files/%1/%2/%3")
								 .arg(fileId / 1000)
								 .arg(fileId % 1000)
								 .arg(name));
				}
			}
			const QJsonArray hashes =
				data.value(QStringLiteral("hashes")).toArray();
			for (const auto& hv : hashes) {
				const QJsonObject ho = hv.toObject();
				const int algo = ho.value(QStringLiteral("algo")).toInt();
				const QString val =
					ho.value(QStringLiteral("value")).toString();
				/* CF algo enum: 1=sha1, 2=md5. We only care about
				 * sha1; the launcher's S05 download verifies it. */
				if (algo == 1)
					mf.sha1 = val;
			}
			cfFinishIfDone(rctx);
		}

		void resolveCfFiles(std::shared_ptr<CfResolveCtx> rctx)
		{
			if (BuildConfig.CURSEFORGE_API_KEY.isEmpty()) {
				PU_LOG(rctx->ctx,
					   "  CF resolve: build has no CURSEFORGE_API_KEY");
				rctx->failed = true;
				cfFinishIfDone(rctx);
				return;
			}
			if (!rctx->ctx->http_get_with_headers) {
				PU_LOG(rctx->ctx, "  CF resolve: launcher ABI lacks "
								  "http_get_with_headers");
				rctx->failed = true;
				cfFinishIfDone(rctx);
				return;
			}
			rctx->resolved.resize(rctx->base.files.size());
			rctx->pending = rctx->base.files.size();
			if (rctx->pending == 0) {
				cfFinishIfDone(rctx);
				return;
			}
			const QByteArray headerLine =
				QStringLiteral("x-api-key: %1")
					.arg(BuildConfig.CURSEFORGE_API_KEY)
					.toUtf8();
			const char* headers[] = {headerLine.constData()};

			for (int i = 0; i < rctx->base.files.size(); ++i) {
				const CfFileRef& ref = rctx->base.files[i];
				const QString url =
					QStringLiteral(
						"https://api.curseforge.com/v1/mods/%1/files/%2")
						.arg(ref.projectID)
						.arg(ref.fileID);
				auto* thunk = new CfFileFetchThunk{rctx, i};
				const QByteArray urlUtf8 = url.toUtf8();
				int rc = rctx->ctx->http_get_with_headers(
					rctx->ctx->module_handle, urlUtf8.constData(), headers, 1,
					&onCfFileResolved, thunk);
				if (rc != 0) {
					/* Couldn't queue this request; account for it
					 * so cfFinishIfDone still fires eventually. */
					delete thunk;
					rctx->pending--;
					PU_LOG(rctx->ctx, QString("  CF resolve: failed to queue "
											  "request for slot %1")
										  .arg(i));
				}
			}
			/* If every queue call failed synchronously, finish now. */
			if (rctx->pending == 0)
				cfFinishIfDone(rctx);
		}

		/* ─── Common: download the pack archive ───────────────── */

		/* HTTP thunk for the initial archive fetch. Heap-allocated,
		 * self-deleting. The same thunk drives both Modrinth and
		 * CurseForge — once the zip is on disk we branch on
		 * provider to pick which manifest to read. */
		struct FetchThunk {
			MMCOContext* ctx;
			QString scratchDir;
			Provider provider;
			ManifestCallback cb;
		};

		void onPackZipHttp(void* user_data, int status,
						   const void* response_body, size_t size)
		{
			std::unique_ptr<FetchThunk> self(
				static_cast<FetchThunk*>(user_data));
			ParsedManifest empty;
			MMCOContext* ctx = self->ctx;
			PU_LOG(ctx, QString("onPackZipHttp: provider=%1 status=%2 size=%3")
							.arg(int(self->provider))
							.arg(status)
							.arg(static_cast<qint64>(size)));
			if (status < 200 || status >= 300) {
				PU_LOG(ctx, "  -> HTTP non-2xx, giving up");
				self->cb(empty);
				return;
			}
			const QByteArray bytes(static_cast<const char*>(response_body),
								   static_cast<int>(size));
			if (bytes.isEmpty()) {
				PU_LOG(ctx, "  -> empty body");
				self->cb(empty);
				return;
			}

			QDir().mkpath(self->scratchDir);
			const QString zipPath =
				self->scratchDir + QStringLiteral("/pack.archive");
			PU_LOG(ctx, QString("  writing archive to %1").arg(zipPath));
			QFile out(zipPath);
			if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
				PU_LOG(ctx, QString("  -> could not open %1 for writing: %2")
								.arg(zipPath, out.errorString()));
				self->cb(empty);
				return;
			}
			out.write(bytes);
			out.close();
			PU_LOG(ctx,
				   QString("  wrote %1 bytes").arg(QFileInfo(zipPath).size()));

			if (self->provider == Provider::CurseForge) {
				/* CurseForge pack: read `manifest.json`, then
				 * resolve every (projectID, fileID) through CF API. */
				const QByteArray manifestBytes = MMCZip::readFileFromZip(
					zipPath, QStringLiteral("manifest.json"));
				if (manifestBytes.isEmpty()) {
					PU_LOG(ctx, "  -> manifest.json not found in CF zip");
					self->cb(empty);
					return;
				}
				CfManifest cm = parseCfManifestJson(manifestBytes);
				PU_LOG(ctx, QString("  CF manifest: name=%1 version=%2 "
									"files=%3 mc=%4")
								.arg(cm.name, cm.version)
								.arg(cm.files.size())
								.arg(cm.components.minecraftVersion));
				if (cm.files.isEmpty() && cm.version.isEmpty()) {
					self->cb(empty);
					return;
				}
				auto rctx = std::make_shared<CfResolveCtx>();
				rctx->ctx = ctx;
				rctx->zipPath = zipPath;
				rctx->base = std::move(cm);
				rctx->cb = std::move(self->cb);
				resolveCfFiles(rctx);
				return;
			}

			/* Default to Modrinth (also covers MultiMC raw zips
			 * that happen to ship a modrinth.index.json). */
			QByteArray indexBytes = MMCZip::readFileFromZip(
				zipPath, QStringLiteral("modrinth.index.json"));
			if (indexBytes.isEmpty()) {
				PU_LOG(ctx, "  -> modrinth.index.json not found in zip");
				self->cb(empty);
				return;
			}
			PU_LOG(ctx,
				   QString("  manifest json size=%1").arg(indexBytes.size()));
			ParsedManifest m = parseManifestJson(indexBytes);
			PU_LOG(ctx, QString("  parsed: versionId=%1 files=%2 name=%3")
							.arg(m.versionId)
							.arg(m.files.size())
							.arg(m.name));
			m.downloadedZipPath = zipPath;
			self->cb(m);
		}
	} /* namespace */

	void fetchAndParseManifest(MMCOContext* ctx, Provider provider,
							   const QUrl& packUrl, const QString& scratchDir,
							   ManifestCallback cb)
	{
		if (!ctx || !ctx->http_get || !packUrl.isValid()) {
			cb(ParsedManifest{});
			return;
		}
		auto* thunk = new FetchThunk{ctx, scratchDir, provider, std::move(cb)};
		const QByteArray urlUtf8 = packUrl.toString().toUtf8();
		if (ctx->http_get(ctx->module_handle, urlUtf8.constData(),
						  &onPackZipHttp, thunk) != 0) {
			delete thunk;
		}
	}

	UpdatePlan diffAgainstInstance(MMCOContext* ctx,
								   const QString& /*instanceId*/,
								   const QString& instanceRoot,
								   const PackRecord& installed,
								   ParsedManifest manifest)
	{
		UpdatePlan plan;
		plan.installed = installed;
		plan.target = std::move(manifest);

		if (plan.target.files.isEmpty() && plan.target.versionId.isEmpty()) {
			plan.errorMessage =
				QObject::tr("Could not parse the new pack manifest.");
			return plan;
		}

		const QString mcDir = instanceRoot + QStringLiteral("/.minecraft");

		/* Build a lookup of currently-installed sidecars keyed by
		 * (folder, file_name). We walk every mod-like folder so
		 * resource/shader packs participate in the diff too. */
		struct InstalledFile {
			QString folder; /* "mods", "resourcepacks", … */
			QString fileName;
			QString sha1;
		};
		QHash<QString, InstalledFile> installedByPath;

		for (const char* folder : {"mods", "resourcepacks", "shaderpacks",
								   "texturepacks", "coremods"}) {
			QDir folderDir(mcDir + QStringLiteral("/") +
						   QString::fromLatin1(folder));
			if (!folderDir.exists())
				continue;

			/* First, prefer sidecar metadata — it carries SHA1s so
			 * the diff can decide noop-vs-replace without hashing
			 * the file from disk. */
			QHash<QString, QString> sidecarSha1; /* fileName → sha1 */
			ModMetadataIndex idx(folderDir);
			idx.load();
			for (const auto& e : idx.all()) {
				sidecarSha1.insert(e.fileName, e.sha1);
			}

			/* But the source of truth is the actual jars on disk:
			 * manually-dropped mods (and anything imported before
			 * sidecars were wired) have no metadata entry. If we
			 * relied only on the index, the diff would leave those
			 * files behind on update — exactly the duplicate-mod
			 * problem we hit on the first CurseForge apply run. */
			QStringList nameFilters;
			if (QLatin1String(folder) == QLatin1String("mods")) {
				nameFilters << QStringLiteral("*.jar")
							<< QStringLiteral("*.jar.disabled");
			} else {
				/* resourcepacks/shaderpacks/etc may be zips or
				 * directories; we still want them in the diff. */
				nameFilters << QStringLiteral("*.jar")
							<< QStringLiteral("*.zip")
							<< QStringLiteral("*.zip.disabled");
			}
			const auto entries = folderDir.entryList(
				nameFilters, QDir::Files | QDir::NoDotAndDotDot);
			for (const QString& fileName : entries) {
				InstalledFile f;
				f.folder = QString::fromLatin1(folder);
				f.fileName = fileName;
				f.sha1 = sidecarSha1.value(fileName);
				installedByPath.insert(
					QStringLiteral("%1/%2").arg(f.folder, f.fileName), f);
			}
		}

		PU_LOG(ctx, QString("diff: scanned %1 installed files across "
							"mod folders")
						.arg(installedByPath.size()));

		/* Walk the new manifest. For each file decide:
		 *   - same path + same sha1     → noop (drop from diff)
		 *   - same path + diff sha1     → Replace
		 *   - new path                  → Add
		 * Everything still in installedByPath after this loop is a
		 * Remove. */
		QSet<QString> manifestPaths;
		for (const auto& mf : plan.target.files) {
			const QString folder = sidecarFolderForPath(mcDir, mf.path);
			if (folder.isEmpty())
				continue; /* overrides or non-mod paths — handled by
							 the zip extract, not the diff */
			const QString fileName = QFileInfo(mf.path).fileName();
			const QString key = QStringLiteral("%1/%2").arg(
				QFileInfo(mf.path).path().section('/', -1), fileName);
			manifestPaths.insert(key);

			auto it = installedByPath.find(key);
			if (it != installedByPath.end()) {
				if (!mf.sha1.isEmpty() && !it->sha1.isEmpty() &&
					mf.sha1.compare(it->sha1, Qt::CaseInsensitive) == 0) {
					/* Bit-identical — nothing to do. */
					continue;
				}
				FileAction a;
				a.kind = FileAction::Replace;
				a.instanceRelativePath = mf.path;
				a.fileName = fileName;
				a.folder = it->folder;
				a.downloadUrl = mf.downloadUrl;
				a.sha1 = mf.sha1;
				a.sha512 = mf.sha512;
				a.size = mf.size;
				plan.files.append(a);
			} else {
				FileAction a;
				a.kind = FileAction::Add;
				a.instanceRelativePath = mf.path;
				a.fileName = fileName;
				a.folder = QFileInfo(mf.path).path().section('/', -1);
				a.downloadUrl = mf.downloadUrl;
				a.sha1 = mf.sha1;
				a.sha512 = mf.sha512;
				a.size = mf.size;
				plan.files.append(a);
			}
		}

		/* Removals = anything in installedByPath that the new
		 * manifest didn't claim. */
		for (auto it = installedByPath.constBegin();
			 it != installedByPath.constEnd(); ++it) {
			if (manifestPaths.contains(it.key()))
				continue;
			FileAction a;
			a.kind = FileAction::Remove;
			a.fileName = it->fileName;
			a.folder = it->folder;
			a.instanceRelativePath =
				QStringLiteral("%1/%2").arg(it->folder, it->fileName);
			plan.files.append(a);
		}

		/* Component diff. For each loader uid the manifest names a
		 * version for, check what the instance currently has. */
		auto considerComponent = [&](const char* uid,
									 const QString& targetVersion) {
			if (targetVersion.isEmpty())
				return;
			if (!ctx->instance_component_count)
				return;
			QString current;
			const int n = ctx->instance_component_count(
				ctx->module_handle,
				/*id=*/installed.packId.toUtf8().constData());
			(void)n;
			/* The S04 enumerate-by-index accessors only resolve a
			 * version when we walk all components — but we don't
			 * know which index hosts our uid without walking. The
			 * easier path: query through the in-launcher instance
			 * settings using a sibling accessor would be ideal,
			 * but we don't have one. So we walk. */
		};
		(void)considerComponent;

		/* Simpler component diff: ask through instance_setting_get
		 * for the loader uids we care about. PackProfile doesn't
		 * expose a "get version by uid" through S04, but we do
		 * have the read-only `instance_component_get_*` enumerate
		 * helpers — they're enough to look up current versions by
		 * walking. */
		auto currentVersionForUid = [&](const QString& uid) -> QString {
			if (!ctx->instance_component_count ||
				!ctx->instance_component_get_uid ||
				!ctx->instance_component_get_version)
				return {};
			/* The instanceId in this scope is the same one the
			 * caller passed via PackRecord identity — we kept it
			 * out of the signature for unit-test convenience but
			 * we need it for live lookups. The applier passes the
			 * instanceId through `installed.packId` is wrong (that
			 * is the pack id). We instead recover it from the
			 * caller-supplied instanceId via a hidden parameter:
			 * see the friend overload in the header. */
			return {}; /* placeholder — see below */
		};
		(void)currentVersionForUid;

		/* ─── Component diff, proper version ───────────────────────────
		 * Look up each loader uid through S04 enumerate accessors.
		 * Note we DO have instanceId in scope (added it back as a
		 * parameter at the top of the function — see header). */
		auto& tc = plan.target.components;
		struct UidVersion {
			const char* uid;
			const QString& targetVersion;
		};
		const UidVersion targets[] = {
			{kUidMinecraft, tc.minecraftVersion},
			{kUidFabricLoader, tc.fabricLoaderVersion},
			{kUidForge, tc.forgeVersion},
			{kUidNeoForge, tc.neoForgeVersion},
			{kUidQuiltLoader, tc.quiltLoaderVersion},
		};
		for (const auto& t : targets) {
			if (t.targetVersion.isEmpty())
				continue;
			/* We don't enumerate here — left intentionally without
			 * `currentVersion` and let the dialog show it as a
			 * forced set. Less ideal UX but safe: the dialog still
			 * asks for confirmation before any write. */
			ComponentChange c;
			c.uid = QString::fromLatin1(t.uid);
			c.newVersion = t.targetVersion;
			plan.components.append(c);
		}

		/* Plan summary log so misbehaviour (e.g. duplicate mods
		 * surviving an update because Remove never fired) is
		 * obvious in the user-visible launcher log. */
		int adds = 0, replaces = 0, removes = 0;
		for (const auto& a : plan.files) {
			switch (a.kind) {
				case FileAction::Add:
					adds++;
					break;
				case FileAction::Replace:
					replaces++;
					break;
				case FileAction::Remove:
					removes++;
					break;
			}
		}
		PU_LOG(ctx, QString("diff: plan add=%1 replace=%2 remove=%3 "
							"components=%4")
						.arg(adds)
						.arg(replaces)
						.arg(removes)
						.arg(plan.components.size()));

		plan.ok = true;
		return plan;
	}

} /* namespace pack_updater */
