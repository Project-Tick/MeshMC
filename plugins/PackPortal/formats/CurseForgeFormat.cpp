/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "CurseForgeFormat.h"

namespace pack::curseforge
{

	namespace
	{
		QString loaderIdFor(const ComponentSet& c)
		{
			if (!c.quiltVersion.isEmpty())
				return QStringLiteral("quilt-%1").arg(c.quiltVersion);
			if (!c.fabricVersion.isEmpty())
				return QStringLiteral("fabric-%1").arg(c.fabricVersion);
			if (!c.forgeVersion.isEmpty())
				return QStringLiteral("forge-%1").arg(c.forgeVersion);
			if (!c.neoForgeVersion.isEmpty()) {
				if (c.minecraftVersion == QStringLiteral("1.20.1"))
					return QStringLiteral("neoforge-1.20.1-%1")
						.arg(c.neoForgeVersion);
				return QStringLiteral("neoforge-%1").arg(c.neoForgeVersion);
			}
			return {};
		}
	} // namespace

	QByteArray buildManifest(const ExportStage& stage, QByteArray* modlistHtml)
	{
		QJsonObject root;
		root["manifestType"] = "minecraftModpack";
		root["manifestVersion"] = 1;
		root["name"] = stage.info.name;
		root["version"] = stage.info.version;
		root["author"] = stage.info.author;
		root["overrides"] = "overrides";

		QJsonObject mc;
		mc["version"] = stage.info.components.minecraftVersion;
		QJsonArray loaders;
		QString lid = loaderIdFor(stage.info.components);
		if (!lid.isEmpty()) {
			QJsonObject loader;
			loader["id"] = lid;
			loader["primary"] = true;
			loaders.append(loader);
		}
		mc["modLoaders"] = loaders;
		root["minecraft"] = mc;

		QJsonArray files;
		QString html = QStringLiteral("<ul>");
		for (const auto& f : stage.files) {
			// A file ends up in manifest.files iff we resolved both
			// projectID and fileID from the launcher's metadata sidecar.
			// Everything else is staged into overrides/ instead — see
			// ExportEngine::stageOverrides() for the matching policy.
			if (!f.curseProjectId.has_value() || !f.curseFileId.has_value())
				continue;

			QJsonObject entry;
			entry["projectID"] = *f.curseProjectId;
			entry["fileID"] = *f.curseFileId;
			entry["required"] = true;
			files.append(entry);

			html +=
				QStringLiteral(
					"<li><a href=\"https://www.curseforge.com/projects/%1\">"
					"%2</a></li>")
					.arg(*f.curseProjectId)
					.arg(f.displayName.toHtmlEscaped());
		}
		html += QStringLiteral("</ul>");
		root["files"] = files;

		if (modlistHtml)
			*modlistHtml = html.toUtf8();

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
		auto root = doc.object();
		if (root.value("manifestType").toString() !=
			QStringLiteral("minecraftModpack")) {
			if (errorMsg)
				*errorMsg = "not a CurseForge minecraftModpack manifest";
			return false;
		}
		info.name = root.value("name").toString();
		info.version = root.value("version").toString();
		info.author = root.value("author").toString();

		auto mc = root.value("minecraft").toObject();
		info.components.minecraftVersion = mc.value("version").toString();
		for (auto v : mc.value("modLoaders").toArray()) {
			auto obj = v.toObject();
			QString id = obj.value("id").toString();
			// Order matters: "neoforge-1.20.1-" starts with "neoforge-"
			// and ALSO starts with "forge-" if you ignore the prefix.
			// Check the longer / more specific prefix first.
			if (id.startsWith(QStringLiteral("neoforge-1.20.1-"))) {
				info.components.neoForgeVersion =
					id.mid(QStringLiteral("neoforge-1.20.1-").size());
			} else if (id.startsWith(QStringLiteral("neoforge-"))) {
				info.components.neoForgeVersion =
					id.mid(QStringLiteral("neoforge-").size());
			} else if (id.startsWith(QStringLiteral("forge-"))) {
				info.components.forgeVersion =
					id.mid(QStringLiteral("forge-").size());
			} else if (id.startsWith(QStringLiteral("fabric-"))) {
				info.components.fabricVersion =
					id.mid(QStringLiteral("fabric-").size());
			} else if (id.startsWith(QStringLiteral("quilt-"))) {
				info.components.quiltVersion =
					id.mid(QStringLiteral("quilt-").size());
			}
		}

		for (auto v : root.value("files").toArray()) {
			auto obj = v.toObject();
			PackFile pf;
			pf.embed = false;
			pf.curseProjectId = obj.value("projectID").toInt();
			pf.curseFileId = obj.value("fileID").toInt();
			// We can't predict the file name without a CurseForge API call;
			// the import engine resolves it via the FileResolvingTask path.
			pf.displayName = QStringLiteral("curseforge:%1/%2")
								 .arg(*pf.curseProjectId)
								 .arg(*pf.curseFileId);
			pf.instancePath = QStringLiteral("mods/") + pf.displayName + ".jar";
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
		return safe + "-" + version + ".zip";
	}

} // namespace pack::curseforge
