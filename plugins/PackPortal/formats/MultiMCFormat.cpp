/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "MultiMCFormat.h"

namespace pack::multimc
{

	QByteArray buildInstanceCfg(const ExportStage& stage)
	{
		// instance.cfg is a simple INI written by INISettingsObject.
		// The keys we emit here are the ones every fresh instance carries
		// on disk (mirrors InstanceCreationTask). Everything else stays
		// at the launcher's defaults after the imported instance is
		// reloaded.
		//
		// Newlines are escaped through QSettings on disk normally; we
		// can't include literal `\n` inside the values here because INI
		// has no escape mechanism. We deliberately don't expose `summary`
		// as `notes` because notes is meant to be free-form Markdown that
		// the user types, not a side-channel for export metadata.
		QString s;
		QTextStream out(&s);
		out << "InstanceType=OneSix\n";
		out << "name=" << stage.info.name << "\n";
		out << "iconKey=" << stage.info.iconKey << "\n";
		if (!stage.info.summary.isEmpty()) {
			// Strip any newlines defensively — INI is line-oriented.
			QString safeSummary = stage.info.summary;
			safeSummary.replace('\n', ' ').replace('\r', ' ');
			out << "notes=" << safeSummary << "\n";
		}
		out << "JoinServerOnLaunch=false\n";
		return s.toUtf8();
	}

	QByteArray buildMmcPackJson(const ExportStage& stage)
	{
		// mmc-pack.json schema is the one PackProfile reads on disk —
		// see launcher/minecraft/PackProfile.cpp (componentFromJsonV1).
		// formatVersion=1, components=[ {uid, version, important?}, ... ].
		// Only `net.minecraft` is marked important; loader components
		// are tracked but not flagged as important so the user can swap
		// them after import without the launcher complaining.
		QJsonObject root;
		root["formatVersion"] = 1;

		QJsonArray components;

		auto addComponent = [&](const QString& uid, const QString& version,
								bool important) {
			if (version.isEmpty())
				return;
			QJsonObject comp;
			comp["uid"] = uid;
			comp["version"] = version;
			if (important)
				comp["important"] = true;
			components.append(comp);
		};

		addComponent(QStringLiteral("net.minecraft"),
					 stage.info.components.minecraftVersion,
					 /*important=*/true);
		addComponent(QStringLiteral("net.minecraftforge"),
					 stage.info.components.forgeVersion, false);
		addComponent(QStringLiteral("net.neoforged"),
					 stage.info.components.neoForgeVersion, false);
		addComponent(QStringLiteral("net.fabricmc.fabric-loader"),
					 stage.info.components.fabricVersion, false);
		addComponent(QStringLiteral("org.quiltmc.quilt-loader"),
					 stage.info.components.quiltVersion, false);

		root["components"] = components;

		// Indented is fine here — mmc-pack.json is the user-facing one
		// on disk and gets edited by hand frequently.
		return QJsonDocument(root).toJson(QJsonDocument::Indented);
	}

	bool parseTree(const QString& rootDir, PackInfo& info, QString* errorMsg)
	{
		QFile cfgFile(QDir(rootDir).filePath("instance.cfg"));
		if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			if (errorMsg)
				*errorMsg = "instance.cfg missing or unreadable";
			return false;
		}
		QString cfgText = QString::fromUtf8(cfgFile.readAll());
		for (const auto& line : cfgText.split('\n', Qt::SkipEmptyParts)) {
			int eq = line.indexOf('=');
			if (eq < 0)
				continue;
			QString key = line.left(eq).trimmed();
			QString value = line.mid(eq + 1).trimmed();
			if (key == QStringLiteral("name"))
				info.name = value;
			else if (key == QStringLiteral("notes"))
				info.summary = value;
		}

		QFile packFile(QDir(rootDir).filePath("mmc-pack.json"));
		if (packFile.open(QIODevice::ReadOnly)) {
			auto doc = QJsonDocument::fromJson(packFile.readAll());
			if (doc.isObject()) {
				for (auto v : doc.object().value("components").toArray()) {
					auto obj = v.toObject();
					QString uid = obj.value("uid").toString();
					QString version = obj.value("version").toString();
					if (uid == QStringLiteral("net.minecraft"))
						info.components.minecraftVersion = version;
					else if (uid == QStringLiteral("net.minecraftforge"))
						info.components.forgeVersion = version;
					else if (uid == QStringLiteral("net.neoforged"))
						info.components.neoForgeVersion = version;
					else if (uid ==
							 QStringLiteral("net.fabricmc.fabric-loader"))
						info.components.fabricVersion = version;
					else if (uid == QStringLiteral("org.quiltmc.quilt-loader"))
						info.components.quiltVersion = version;
				}
			}
		}
		return true;
	}

	QString suggestedFileName(const PackInfo& info)
	{
		QString safe =
			info.name.isEmpty() ? QStringLiteral("instance") : info.name;
		safe.replace(QRegularExpression(R"([^A-Za-z0-9_.-])"), "_");
		return safe + ".zip";
	}

} // namespace pack::multimc
