/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ExportEngine.h"
#include "formats/MrPackFormat.h"
#include "formats/CurseForgeFormat.h"
#include "formats/MultiMCFormat.h"

#include <QCryptographicHash>
#include <QStandardPaths>
#include <QUuid>

// The SDK header forward-declares PackProfile but never includes its
// definition. We call getComponentVersion() through a shared_ptr, so
// we need the complete type here. Net::Mode is the parameter type for
// reload() — we want the Offline variant so the export never blocks
// on a network round-trip.
#include "minecraft/PackProfile.h"
#include "net/Mode.h"

namespace pack
{

namespace
{
QString uniqueStageDir(const QString& tag)
{
	QString base = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
	if (base.isEmpty())
		base = QDir::tempPath();
	QString name = QStringLiteral("packportal-%1-%2")
					   .arg(tag, QUuid::createUuid()
									 .toString(QUuid::Id128)
									 .left(8));
	QString d = QDir(base).filePath(name);
	QDir().mkpath(d);
	return d;
}

QByteArray hashFile(const QString& path, QCryptographicHash::Algorithm alg)
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly))
		return {};
	QCryptographicHash h(alg);
	h.addData(&f);
	return h.result();
}

QString gameRootOf(const QString& instanceRoot)
{
	return QDir(instanceRoot).filePath(QStringLiteral(".minecraft"));
}
} // namespace

bool ExportEngine::collectComponents(const QString& instanceId,
									 ComponentSet& out)
{
	auto instList = APPLICATION->instances();
	if (!instList)
		return false;
	auto inst = instList->getInstanceById(instanceId);
	if (!inst)
		return false;
	auto mc = std::dynamic_pointer_cast<MinecraftInstance>(inst);
	if (!mc)
		return false;

	auto profile = mc->getPackProfile();
	if (!profile)
		return false;

	profile->reload(Net::Mode::Offline);

	out.minecraftVersion = profile->getComponentVersion("net.minecraft");
	out.forgeVersion = profile->getComponentVersion("net.minecraftforge");
	out.neoForgeVersion = profile->getComponentVersion("net.neoforged");
	out.fabricVersion =
		profile->getComponentVersion("net.fabricmc.fabric-loader");
	out.quiltVersion =
		profile->getComponentVersion("org.quiltmc.quilt-loader");
	return true;
}

bool ExportEngine::collectFiles(const QString& /*instanceId*/,
								const QString& root, const Options& opts,
								QList<PackFile>& out, QStringList& warnings)
{
	QString gameRoot = gameRootOf(root);
	QDir gameDir(gameRoot);
	if (!gameDir.exists()) {
		warnings.append(
			QStringLiteral(".minecraft not found inside instance"));
		return false;
	}

	const QStringList alwaysSkip{
		"logs",			   "crash-reports", "screenshots",
		"saves/.lockfile", "natives",		"usercache.json",
		"usernamecache.json", "replay_pid",	".fabric",
		".mixin.out",	   "hs_err_pid",
	};

	QDirIterator it(gameRoot, QDir::Files | QDir::NoDotAndDotDot,
					QDirIterator::Subdirectories);
	while (it.hasNext()) {
		it.next();
		QString rel = gameDir.relativeFilePath(it.filePath());

		if (opts.skipVolatile) {
			bool skip = false;
			for (const auto& p : alwaysSkip) {
				if (rel == p || rel.startsWith(p + "/") ||
					rel.contains("/" + p + "/")) {
					skip = true;
					break;
				}
			}
			if (skip)
				continue;
		}
		if (opts.skipWorlds && rel.startsWith("saves/"))
			continue;

		PackFile pf;
		pf.instancePath = rel;
		pf.displayName = QFileInfo(rel).fileName();
		pf.embed = true;
		out.append(pf);
	}
	return true;
}

void ExportEngine::resolveExternalIdentity(QList<PackFile>& files,
										   const QString& instanceRoot)
{

	const QString gameRoot = gameRootOf(instanceRoot);
	const QStringList trackedFolders = {
		QStringLiteral("mods"),			QStringLiteral("coremods"),
		QStringLiteral("resourcepacks"), QStringLiteral("texturepacks"),
		QStringLiteral("shaderpacks"),
	};

	// Pre-build a filename → PackFile* map so the inner loop is O(1).
	QHash<QString, PackFile*> byFileName;
	byFileName.reserve(files.size());
	for (auto& pf : files)
		byFileName.insert(QFileInfo(pf.instancePath).fileName(), &pf);

	for (const QString& folder : trackedFolders) {
		QDir indexDir(QDir(gameRoot).filePath(folder + "/.index"));
		if (!indexDir.exists())
			continue;

		for (const auto& fi :
			 indexDir.entryInfoList({"*.json"}, QDir::Files, QDir::Name)) {
			QFile f(fi.absoluteFilePath());
			if (!f.open(QIODevice::ReadOnly))
				continue;
			QJsonParseError jerr;
			const auto doc = QJsonDocument::fromJson(f.readAll(), &jerr);
			if (jerr.error != QJsonParseError::NoError || !doc.isObject())
				continue;
			const auto o = doc.object();

			// Sidecar stem = file name on disk:
			// mods/.index/sodium.jar.json → sodium.jar
			const QString fileName = fi.completeBaseName();

			auto it = byFileName.constFind(fileName);
			if (it == byFileName.constEnd()) {
				// Mod may be currently `.disabled`; try that variant.
				it = byFileName.constFind(
					fileName + QStringLiteral(".disabled"));
				if (it == byFileName.constEnd())
					continue;
			}

			PackFile* pf = it.value();
			const QString platform =
				o.value("platform").toString().toLower();
			const QString projectId = o.value("projectId").toString();
			const QString versionId = o.value("versionId").toString();
			const QString url = o.value("downloadUrl").toString();
			const QString sha1Hex = o.value("sha1").toString().toLower();

			if (platform == QStringLiteral("curseforge")) {
				bool okP = false, okV = false;
				const int p = projectId.toInt(&okP);
				const int v = versionId.toInt(&okV);
				if (okP && okV && p > 0 && v > 0) {
					pf->curseProjectId = p;
					pf->curseFileId = v;
				}
			} else if (platform == QStringLiteral("modrinth")) {
				pf->modrinthSlug = projectId;
				pf->modrinthVersionId = versionId;
			}
			if (!url.isEmpty())
				pf->downloadUrl = QUrl(url, QUrl::StrictMode);

			if (!sha1Hex.isEmpty())
				pf->sha1 = QByteArray::fromHex(sha1Hex.toLatin1());
		}
	}
}

bool ExportEngine::hashFiles(ExportStage& stage)
{

	const QString gameRoot = gameRootOf(stage.instanceRoot);
	for (auto& f : stage.files) {
		const QString abs = QDir(gameRoot).filePath(f.instancePath);
		QFileInfo fi(abs);
		if (!fi.exists())
			continue;
		f.sizeBytes = fi.size();

		const bool willBeReferenced =
			f.curseProjectId.has_value() ||
			!f.modrinthVersionId.isEmpty() || !f.downloadUrl.isEmpty();
		if (!willBeReferenced)
			continue;
		if (f.sha1.isEmpty())
			f.sha1 = hashFile(abs, QCryptographicHash::Sha1);
		f.sha512 = hashFile(abs, QCryptographicHash::Sha512);
	}
	return true;
}

bool ExportEngine::stageOverrides(ExportStage& stage, const Options& opts,
								  QStringList& warnings)
{
	QString gameRoot = gameRootOf(stage.instanceRoot);
	QString overridesRoot;
	switch (opts.format) {
		case Format::MrPack:
			overridesRoot = QDir(stage.stageDir).filePath("overrides");
			break;
		case Format::CurseForge:
			overridesRoot = QDir(stage.stageDir).filePath("overrides");
			break;
		case Format::MultiMC:
			overridesRoot = QDir(stage.stageDir).filePath(".minecraft");
			break;
	}
	QDir().mkpath(overridesRoot);
	for (auto& f : stage.files) {
		bool referenced = false;
		if (!opts.embedAllMods && opts.format != Format::MultiMC) {
			switch (opts.format) {
				case Format::MrPack:
					referenced = !f.downloadUrl.isEmpty();
					break;
				case Format::CurseForge:
					referenced = f.curseProjectId.has_value() &&
								 f.curseFileId.has_value();
					break;
				case Format::MultiMC:
					break; // unreachable
			}
		}
		f.embed = !referenced;
		if (referenced)
			continue;

		const QString abs = QDir(gameRoot).filePath(f.instancePath);
		if (!QFileInfo::exists(abs)) {
			warnings.append(
				QStringLiteral("Source missing: %1").arg(f.instancePath));
			continue;
		}
		const QString dst = QDir(overridesRoot).filePath(f.instancePath);
		QDir().mkpath(QFileInfo(dst).absolutePath());
		if (!QFile::copy(abs, dst)) {
			warnings.append(
				QStringLiteral("Failed to stage %1").arg(f.instancePath));
		}
	}
	return true;
}

bool ExportEngine::writeFile(const QString& abs, const QByteArray& bytes)
{
	QDir().mkpath(QFileInfo(abs).absolutePath());
	QFile f(abs);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;
	f.write(bytes);
	return true;
}

ExportEngine::Result
ExportEngine::exportInstance(const QString& instanceId, const Options& opts,
							 const QString& outputPath)
{
	Result r;

	auto instList = APPLICATION->instances();
	if (!instList) {
		r.errorMsg = "Instance list unavailable";
		return r;
	}
	auto inst = instList->getInstanceById(instanceId);
	if (!inst) {
		r.errorMsg = "Instance not found";
		return r;
	}

	ExportStage stage;
	stage.instanceRoot = inst->instanceRoot();
	stage.info.name =
		opts.name.isEmpty() ? inst->name() : opts.name;
	stage.info.version =
		opts.version.isEmpty() ? QStringLiteral("1.0.0") : opts.version;
	stage.info.author = opts.author;
	stage.info.summary = opts.summary;
	stage.info.iconKey = inst->iconKey();
	if (stage.info.iconKey.isEmpty())
		stage.info.iconKey = QStringLiteral("default");
	if (!collectComponents(instanceId, stage.info.components)) {
		r.errorMsg = "Could not read instance components";
		return r;
	}

	if (stage.info.components.minecraftVersion.trimmed().isEmpty()) {
		r.errorMsg = QStringLiteral(
			"This instance has no resolved Minecraft version yet. "
			"Open it once in the launcher (Edit Instance → Version) "
			"to let the version components settle, then try the "
			"export again.");
		return r;
	}

	if (!collectFiles(instanceId, stage.instanceRoot, opts, stage.files,
					  r.warnings)) {
		r.errorMsg = "Could not enumerate instance files";
		return r;
	}
	resolveExternalIdentity(stage.files, stage.instanceRoot);
	hashFiles(stage);

	stage.stageDir = uniqueStageDir(
		opts.format == Format::MrPack	  ? "mrpack"
		: opts.format == Format::CurseForge ? "cf"
											: "multimc");

	if (!stageOverrides(stage, opts, r.warnings)) {
		r.errorMsg = "Failed to copy overrides";
		return r;
	}

	// Format-specific manifest emission
	switch (opts.format) {
		case Format::MrPack: {
			auto bytes = mrpack::buildManifest(stage);
			writeFile(QDir(stage.stageDir).filePath("modrinth.index.json"),
					  bytes);
			break;
		}
		case Format::CurseForge: {
			QByteArray html;
			auto bytes = curseforge::buildManifest(stage, &html);
			writeFile(QDir(stage.stageDir).filePath("manifest.json"), bytes);
			writeFile(QDir(stage.stageDir).filePath("modlist.html"), html);
			break;
		}
		case Format::MultiMC: {
			writeFile(QDir(stage.stageDir).filePath("instance.cfg"),
					  multimc::buildInstanceCfg(stage));
			writeFile(QDir(stage.stageDir).filePath("mmc-pack.json"),
					  multimc::buildMmcPackJson(stage));
			break;
		}
	}

	// Accounting
	for (const auto& f : stage.files) {
		if (f.embed)
			++r.filesEmbedded;
		else
			++r.filesReferenced;
	}

	if (!MMCZip::compressDir(
			outputPath, stage.stageDir,
			[](const QString&) -> bool { return false; })) {
		r.errorMsg = "Failed to compress staging directory";
		QDir(stage.stageDir).removeRecursively();
		return r;
	}
	QDir(stage.stageDir).removeRecursively();

	r.ok = true;
	r.outputPath = outputPath;
	return r;
}

} // namespace pack
