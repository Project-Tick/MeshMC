/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "MrPackFormat.h"

namespace pack::mrpack
{

	QByteArray buildManifest(const ExportStage& stage)
	{
		// Spec reference (read 2026-05-15):
		//   https://support.modrinth.com/en/articles/8802351-modrinth-modpack-format-mrpack
		//
		// Required top-level fields: formatVersion (=1), game (="minecraft"),
		// name, versionId, files (array, may be empty), dependencies
		// (object; must include "minecraft").
		//
		// File entries: path, hashes.{sha1, sha512}, downloads (array of
		// HTTPS URLs), fileSize. env.{client,server} is OPTIONAL — when
		// omitted, the launcher treats the file as required on both sides.
		QJsonObject root;
		root["formatVersion"] = 1;
		root["game"] = "minecraft";
		root["name"] = stage.info.name;
		root["versionId"] = stage.info.version;
		if (!stage.info.summary.isEmpty())
			root["summary"] = stage.info.summary;

		QJsonArray files;
		for (const auto& f : stage.files) {
			// Only resolved external references end up in the manifest.
			// Everything else lives under overrides/ — see the matching
			// embed/reference policy in ExportEngine::stageOverrides().
			if (f.embed || f.downloadUrl.isEmpty())
				continue;

			QJsonObject entry;
			entry["path"] = f.instancePath;

			// Modrinth requires SHA-1 + SHA-512. Anything else is ignored
			// by the spec ("Other hashes are optional, but will usually be
			// ignored"). hashFiles() recomputes both for every referenced
			// file, so both should be non-empty here.
			QJsonObject hashes;
			hashes["sha1"] = QString::fromLatin1(f.sha1.toHex());
			hashes["sha512"] = QString::fromLatin1(f.sha512.toHex());
			entry["hashes"] = hashes;

			// env (optional but recommended). Modrinth recognises three
			// values per side: required / optional / unsupported.
			QJsonObject env;
			env["client"] = f.serverOnly ? "unsupported" : "required";
			env["server"] = f.clientOnly ? "unsupported" : "required";
			entry["env"] = env;

			// downloads MUST be an array of HTTPS URLs without unencoded
			// spaces (RFC 3986 compliance). QUrl::toEncoded gives us
			// percent-encoded output that satisfies the spec.
			entry["downloads"] = QJsonArray{QString::fromLatin1(
				f.downloadUrl.toEncoded(QUrl::FullyEncoded))};
			entry["fileSize"] = qint64(f.sizeBytes);

			files.append(entry);
		}
		root["files"] = files;

		// Dependencies. The spec only documents a fixed set of IDs —
		// minecraft, forge, neoforge, fabric-loader, quilt-loader — and
		// explicitly tells implementers to expect more in the future.
		// We never emit something the spec didn't define.
		QJsonObject deps;
		deps["minecraft"] = stage.info.components.minecraftVersion;
		if (!stage.info.components.forgeVersion.isEmpty())
			deps["forge"] = stage.info.components.forgeVersion;
		if (!stage.info.components.neoForgeVersion.isEmpty())
			deps["neoforge"] = stage.info.components.neoForgeVersion;
		if (!stage.info.components.fabricVersion.isEmpty())
			deps["fabric-loader"] = stage.info.components.fabricVersion;
		if (!stage.info.components.quiltVersion.isEmpty())
			deps["quilt-loader"] = stage.info.components.quiltVersion;
		root["dependencies"] = deps;

		return QJsonDocument(root).toJson(QJsonDocument::Compact);
	}

	bool parseManifest(const QByteArray& json, PackInfo& info,
					   QList<PackFile>& outFiles, QString* errorMsg)
	{
		QJsonParseError err;
		auto doc = QJsonDocument::fromJson(json, &err);
		if (err.error != QJsonParseError::NoError) {
			if (errorMsg)
				*errorMsg = err.errorString();
			return false;
		}
		if (!doc.isObject()) {
			if (errorMsg)
				*errorMsg = "manifest root is not an object";
			return false;
		}
		auto root = doc.object();
		if (root.value("game").toString() != QStringLiteral("minecraft")) {
			if (errorMsg)
				*errorMsg = "not a minecraft mrpack";
			return false;
		}
		if (root.value("formatVersion").toInt(0) != 1) {
			if (errorMsg)
				*errorMsg = "unsupported mrpack formatVersion";
			return false;
		}

		info.name = root.value("name").toString();
		info.version = root.value("versionId").toString();
		info.summary = root.value("summary").toString();

		auto deps = root.value("dependencies").toObject();
		info.components.minecraftVersion = deps.value("minecraft").toString();
		info.components.forgeVersion = deps.value("forge").toString();
		info.components.neoForgeVersion = deps.value("neoforge").toString();
		info.components.fabricVersion = deps.value("fabric-loader").toString();
		info.components.quiltVersion = deps.value("quilt-loader").toString();

		auto filesArr = root.value("files").toArray();
		for (auto v : filesArr) {
			auto obj = v.toObject();
			PackFile pf;
			pf.embed = false;
			pf.instancePath = obj.value("path").toString();
			pf.displayName = QFileInfo(pf.instancePath).fileName();
			pf.sizeBytes = qint64(obj.value("fileSize").toDouble(0));
			auto hashes = obj.value("hashes").toObject();
			QString sha1 = hashes.value("sha1").toString();
			QString sha512 = hashes.value("sha512").toString();
			if (!sha1.isEmpty())
				pf.sha1 = QByteArray::fromHex(sha1.toLatin1());
			if (!sha512.isEmpty())
				pf.sha512 = QByteArray::fromHex(sha512.toLatin1());

			auto envObj = obj.value("env").toObject();
			QString clientReq = envObj.value("client").toString();
			QString serverReq = envObj.value("server").toString();
			pf.clientOnly = (serverReq == QStringLiteral("unsupported"));
			pf.serverOnly = (clientReq == QStringLiteral("unsupported"));

			auto downloads = obj.value("downloads").toArray();
			if (!downloads.isEmpty())
				pf.downloadUrl = QUrl(downloads.first().toString());

			outFiles.append(pf);
		}
		return true;
	}

	QString suggestedFileName(const PackInfo& info)
	{
		QString safe =
			info.name.isEmpty() ? QStringLiteral("modpack") : info.name;
		safe.replace(QRegularExpression(R"([^A-Za-z0-9_.-])"), "_");
		QString version =
			info.version.isEmpty() ? QStringLiteral("1.0.0") : info.version;
		version.replace(QRegularExpression(R"([^A-Za-z0-9_.-])"), "_");
		return safe + "-" + version + ".mrpack";
	}

} // namespace pack::mrpack
