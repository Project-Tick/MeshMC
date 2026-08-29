/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 */

#include "ModrinthContentModel.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <utility>

#include "HoeDown.h"
#include "Json.h"
#include "modplatform/modrinth/ModrinthApi.h"

namespace
{

	/* One Modrinth project object into one result.
	 *
	 * A search hit and the reply from /project/{id} carry the same field
	 * names, with two differences: the hit calls the id "project_id" and
	 * carries an "author", the project carries neither. Both are handled
	 * here so the two entry points cannot drift apart. Returns false when
	 * the object is missing something a row cannot do without. */
	bool parseProjectObject(const QJsonObject& obj,
							ModPlatform::IndexedProject& project)
	{
		try {
			project.projectId = Json::ensureString(obj, "project_id", "");
			if (project.projectId.isEmpty()) {
				project.projectId = Json::requireString(obj, "id");
			}
			project.slug = Json::ensureString(obj, "slug", "");
			project.name = Json::requireString(obj, "title");
			project.description = Json::ensureString(obj, "description", "");
			project.author = Json::ensureString(obj, "author", "");
			project.logoUrl = Json::ensureString(obj, "icon_url", "");
			/* The slug is a stable, filesystem-safe cache key; the title
			 * is neither. */
			project.logoKey =
				project.slug.isEmpty() ? project.projectId : project.slug;

			if (!project.slug.isEmpty()) {
				project.websiteUrl =
					QStringLiteral("https://modrinth.com/project/") +
					project.slug;
			}
		} catch (const JSONValidationError& e) {
			qWarning() << "Error loading Modrinth project:" << e.cause();
			return false;
		}
		return true;
	}

} // namespace

ModrinthContentModel::ModrinthContentModel(
	ModPlatform::ContentType contentType, ModPlatform::SearchFilters filters,
	QObject* parent)
	: ContentProviderModel(ModrinthApi::get(), contentType, std::move(filters),
						   parent)
{
}

QString ModrinthContentModel::iconCacheName() const
{
	return QStringLiteral("ModrinthModIcons");
}

QList<ModPlatform::Category>
ModrinthContentModel::parseCategoriesResponse(const QByteArray& bytes) const
{
	QList<ModPlatform::Category> categories;

	QJsonParseError parseError{};
	const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
	if (parseError.error != QJsonParseError::NoError) {
		qWarning() << "Error parsing Modrinth category response:"
				   << parseError.errorString();
		return categories;
	}

	/* One list covers the whole site, so the entries for other kinds of
	 * content have to be dropped here. */
	const QString wanted =
		ModPlatform::contentTypeToModrinthFacet(contentType());

	const QJsonArray entries = doc.array();
	for (const auto& entryRaw : entries) {
		const auto obj = entryRaw.toObject();
		if (Json::ensureString(obj, "project_type", "") != wanted) {
			continue;
		}
		try {
			/* Modrinth searches by the name itself, so there is no
			 * separate id to carry. */
			const QString name = Json::requireString(obj, "name");
			categories.append({name, name});
		} catch (const Json::JsonException& e) {
			qWarning() << "Skipping malformed Modrinth category:" << e.what();
		}
	}

	return categories;
}

QList<ModPlatform::IndexedProject>
ModrinthContentModel::parseSearchResponse(const QByteArray& bytes,
										  int& totalHits) const
{
	QList<ModPlatform::IndexedProject> results;

	QJsonParseError parseError{};
	const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
	if (parseError.error != QJsonParseError::NoError) {
		qWarning() << "Error parsing Modrinth search response:"
				   << parseError.errorString();
		return results;
	}

	const auto root = doc.object();
	for (const auto& hitRaw : Json::ensureArray(root, "hits")) {
		ModPlatform::IndexedProject project;
		if (parseProjectObject(hitRaw.toObject(), project)) {
			results.append(project);
		}
	}

	totalHits = Json::ensureInteger(root, "total_hits", -1);

	return results;
}

ModPlatform::IndexedProject
ModrinthContentModel::parseProjectResponse(const QByteArray& bytes) const
{
	ModPlatform::IndexedProject project;

	QJsonParseError parseError{};
	const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
	if (parseError.error != QJsonParseError::NoError) {
		qWarning() << "Error parsing Modrinth project response:"
				   << parseError.errorString();
		return project;
	}
	if (!doc.isObject()) {
		return project;
	}

	if (!parseProjectObject(doc.object(), project)) {
		return ModPlatform::IndexedProject();
	}
	return project;
}

QList<ModPlatform::ContentVersion> ModrinthContentModel::parseVersionsResponse(
	const QByteArray& bytes, const ModPlatform::IndexedProject& project) const
{
	Q_UNUSED(project)

	QList<ModPlatform::ContentVersion> versions;

	const QJsonDocument doc = QJsonDocument::fromJson(bytes);
	if (!doc.isArray()) {
		return versions;
	}

	for (const auto& versionRaw : doc.array()) {
		const auto versionObj = versionRaw.toObject();

		ModPlatform::ContentVersion version;
		version.versionId = Json::ensureString(versionObj, "id", "");
		version.versionType =
			Json::ensureString(versionObj, "version_type", "");

		const QString name = Json::ensureString(versionObj, "name", "");
		const QString number =
			Json::ensureString(versionObj, "version_number", "");
		version.name =
			name.isEmpty() ? number
						   : (number.isEmpty()
								  ? name
								  : QString("%1 (%2)").arg(name, number));

		const auto files = Json::ensureArray(versionObj, "files");
		for (const auto& fileRaw : files) {
			const auto fileObj = fileRaw.toObject();
			const bool primary = Json::ensureBoolean(fileObj, "primary", false);
			if (!primary && files.size() != 1) {
				continue;
			}
			version.downloadUrl = Json::ensureString(fileObj, "url", "");
			version.fileName = Json::ensureString(fileObj, "filename", "");
			version.fileSize = Json::ensureInteger(fileObj, "size", 0);
			version.sha1 = Json::ensureString(
				Json::ensureObject(fileObj, "hashes"), "sha1", "");
			break;
		}

		/* A version with no downloadable file is of no use to anyone. */
		if (version.downloadUrl.isEmpty()) {
			continue;
		}

		versions.append(version);
	}

	return versions;
}

QString ModrinthContentModel::parseBodyResponse(const QByteArray& bytes) const
{
	/* Modrinth hands out Markdown, so it has to be rendered here - the
	 * page shows the two providers' descriptions in the same browser. */
	const QJsonDocument doc = QJsonDocument::fromJson(bytes);
	if (!doc.isObject()) {
		return QString();
	}

	const QString body = Json::ensureString(doc.object(), "body", "");
	if (body.isEmpty()) {
		return QString();
	}

	HoeDown renderer;
	return renderer.process(body.toUtf8());
}
