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

#include "ModrinthApi.h"

#include <QObject>

namespace
{

	/* The sort field for a UI index, clamped to the published list. */
	QString sortValueAt(const QList<ModPlatform::SortingMethod>& sorts,
						int sortIndex)
	{
		if (sortIndex >= 0 && sortIndex < sorts.size()) {
			return sorts.at(sortIndex).apiValue;
		}
		return sorts.first().apiValue;
	}

	/* Modrinth facets are a JSON array of arrays: the outer array is
	 * ANDed, each inner array is ORed. Ticking two loaders therefore
	 * has to produce one group with both in it, not two groups. */
	QString facetGroups(const QList<QStringList>& groups)
	{
		QStringList rendered;
		rendered.reserve(groups.size());
		for (const QStringList& group : groups) {
			if (group.isEmpty()) {
				continue;
			}
			QStringList quoted;
			quoted.reserve(group.size());
			for (const QString& term : group) {
				quoted.append(QString("\"%1\"").arg(term));
			}
			rendered.append(QStringLiteral("[") +
							quoted.join(QLatin1Char(',')) +
							QStringLiteral("]"));
		}
		return QStringLiteral("[") + rendered.join(QLatin1Char(',')) +
			   QStringLiteral("]");
	}

	/* The common case: every term stands alone and all of them must
	 * match. */
	QString facetList(const QStringList& terms)
	{
		QList<QStringList> groups;
		groups.reserve(terms.size());
		for (const QString& term : terms) {
			groups.append(QStringList{term});
		}
		return facetGroups(groups);
	}

	/* Turn the environment choice into facet groups.
	 *
	 * Modrinth records how a project relates to each side separately, so
	 * "client side" is not one term but two conditions: it has to do
	 * something on the client, and it must not be required on the
	 * server. Each choice therefore adds two ANDed groups. The reference
	 * launcher sends exactly these terms.
	 *
	 * `Universal` is the strict reading - required on both - rather than
	 * "works on either", which is what no filter at all already gives. */
	void appendSideFacets(QList<QStringList>& facets,
						  ModPlatform::SideFilter side)
	{
		switch (side) {
			case ModPlatform::SideFilter::Client:
				facets.append({QStringLiteral("client_side:required"),
							   QStringLiteral("client_side:optional")});
				facets.append({QStringLiteral("server_side:optional"),
							   QStringLiteral("server_side:unsupported")});
				break;
			case ModPlatform::SideFilter::Server:
				facets.append({QStringLiteral("server_side:required"),
							   QStringLiteral("server_side:optional")});
				facets.append({QStringLiteral("client_side:optional"),
							   QStringLiteral("client_side:unsupported")});
				break;
			case ModPlatform::SideFilter::Universal:
				facets.append({QStringLiteral("client_side:required")});
				facets.append({QStringLiteral("server_side:required")});
				break;
			case ModPlatform::SideFilter::Any:
				break;
		}
	}

	QString quotedJsonArray(const QStringList& values)
	{
		QStringList quoted;
		quoted.reserve(values.size());
		for (const QString& value : values) {
			quoted.append(QString("\"%1\"").arg(value));
		}
		return QStringLiteral("[") + quoted.join(QLatin1Char(',')) +
			   QStringLiteral("]");
	}

	void appendParam(QString& url, const QString& param)
	{
		url += (url.contains(QLatin1Char('?')) ? QLatin1Char('&')
											   : QLatin1Char('?'));
		url += param;
	}

} // namespace

const ModrinthApi& ModrinthApi::get()
{
	static const ModrinthApi instance;
	return instance;
}

QString ModrinthApi::apiHost()
{
	return QStringLiteral("api.modrinth.com");
}

QString ModrinthApi::apiBase()
{
	return QStringLiteral("https://") + apiHost() + QStringLiteral("/v2");
}

QString ModrinthApi::id() const
{
	return QStringLiteral("modrinth");
}

QString ModrinthApi::displayName() const
{
	return QStringLiteral("Modrinth");
}

int ModrinthApi::searchPageSize() const
{
	return 20;
}

QList<ModPlatform::SortingMethod> ModrinthApi::sortingMethods() const
{
	/* Modrinth names its sorts instead of numbering them, and offers a
	 * different set from CurseForge - "relevance" and "follows" have no
	 * counterpart there, "featured" and "category" none here. */
	return {
		{QStringLiteral("relevance"), QObject::tr("Sort by Relevance")},
		{QStringLiteral("downloads"), QObject::tr("Sort by Downloads")},
		{QStringLiteral("follows"), QObject::tr("Sort by Follows")},
		{QStringLiteral("newest"), QObject::tr("Sort by Newest")},
		{QStringLiteral("updated"), QObject::tr("Sort by Last Updated")},
	};
}

QUrl ModrinthApi::searchUrl(const ModPlatform::SearchQuery& query) const
{
	QList<QStringList> facets;
	facets.append(QStringList{
		QStringLiteral("project_type:") +
		ModPlatform::contentTypeToModrinthFacet(query.contentType)});

	if (!query.filters.mcVersions.isEmpty()) {
		QStringList versionFacets;
		versionFacets.reserve(query.filters.mcVersions.size());
		for (const QString& version : query.filters.mcVersions) {
			versionFacets.append(QStringLiteral("versions:") + version);
		}
		/* One group, so several ticked versions mean "any of these".
		 * ANDing them would ask for a project built for all of them at
		 * once, which is not what ticking two versions means. */
		facets.append(versionFacets);
	}

	/* Modrinth models the loader as a category rather than a dedicated
	 * field. Sending it for a resource pack or data pack matches nothing
	 * and silently returns an empty page, so only mods get it. */
	if (!query.filters.loaders.isEmpty() &&
		ModPlatform::contentTypeUsesLoader(query.contentType)) {
		QStringList loaderFacets;
		loaderFacets.reserve(query.filters.loaders.size());
		for (const QString& loader : query.filters.loaders) {
			loaderFacets.append(QStringLiteral("categories:") + loader);
		}
		/* One group, so several ticked loaders mean "any of these". */
		facets.append(loaderFacets);
	}

	if (!query.filters.categoryIds.isEmpty()) {
		QStringList categoryFacets;
		categoryFacets.reserve(query.filters.categoryIds.size());
		for (const QString& category : query.filters.categoryIds) {
			categoryFacets.append(QStringLiteral("categories:") + category);
		}
		/* One group, so ticking two categories means "either of these".
		 * Modrinth files the loader under categories as well, which is
		 * why the loader group above looks the same. */
		facets.append(categoryFacets);
	}

	appendSideFacets(facets, query.filters.side);

	if (query.filters.openSourceOnly) {
		facets.append(QStringList{QStringLiteral("open_source:true")});
	}

	/* One multi-arg call rather than a chain: the search term is
	 * user-supplied, and a chained arg() rescans the string it has
	 * already built, so a term containing "%4" would be mistaken for the
	 * next placeholder. The multi-arg form substitutes in one pass and
	 * never looks at what it inserted. */
	QString url = QString("%1/search?"
						  "query=%2&"
						  "index=%3&"
						  "offset=%4&"
						  "limit=%5")
					  .arg(apiBase(),
						   ModPlatform::encodeSearchTerm(query.term),
						   sortValueAt(sortingMethods(), query.sortIndex),
						   QString::number(query.offset),
						   QString::number(searchPageSize()));
	url += QStringLiteral("&facets=") + facetGroups(facets);

	return QUrl(url);
}

QUrl ModrinthApi::projectUrl(const QString& projectId) const
{
	return QUrl(QString("%1/project/%2").arg(apiBase(), projectId));
}

QUrl ModrinthApi::projectBodyUrl(const QString& projectId) const
{
	/* Modrinth has no separate description endpoint: the long body is a
	 * Markdown field of the project itself. */
	return projectUrl(projectId);
}

QUrl ModrinthApi::categoriesUrl(ModPlatform::ContentType contentType) const
{
	/* One list for the whole site; the reply says which project type
	 * each entry belongs to, so the filtering happens while parsing. */
	Q_UNUSED(contentType)
	return QUrl(QString("%1/tag/category").arg(apiBase()));
}

QUrl ModrinthApi::projectPageUrl(const QString& projectId) const
{
	/* `/mod/` works for every kind of project the site hosts - it
	 * redirects to the right section - so one form covers resource
	 * packs and shaders too. */
	return QUrl(QStringLiteral("https://modrinth.com/mod/") + projectId);
}

QUrl ModrinthApi::projectVersionsUrl(
	const ModPlatform::VersionQuery& query) const
{
	QString url =
		QString("%1/project/%2/version").arg(apiBase(), query.projectId);

	/* Modrinth takes the whole list here and treats it as "any of
	 * these", so a filter naming several versions needs no special
	 * handling - unlike CurseForge, which only looks at the first. */
	if (!query.mcVersions.isEmpty()) {
		appendParam(url, QStringLiteral("game_versions=") +
							 quotedJsonArray(query.mcVersions));
	}

	/* Unlike CurseForge, Modrinth takes the whole list here and treats it
	 * as "any of these", so several ticked loaders need no special
	 * handling. */
	if (!query.loaders.isEmpty() &&
		ModPlatform::contentTypeUsesLoader(query.contentType)) {
		appendParam(url, QStringLiteral("loaders=") +
							 quotedJsonArray(query.loaders));
	}

	return QUrl(url);
}

QUrl ModrinthApi::versionUrl(const QString& versionId)
{
	return QUrl(QString("%1/version/%2").arg(apiBase(), versionId));
}

QUrl ModrinthApi::projectVersionsUrlForLoaders(const QString& projectId,
											   const QStringList& loaders)
{
	QString url = QString("%1/project/%2/version").arg(apiBase(), projectId);
	if (!loaders.isEmpty()) {
		appendParam(url, QStringLiteral("loaders=") + quotedJsonArray(loaders));
	}
	return QUrl(url);
}

QUrl ModrinthApi::modpackSearchUrl(const QString& term, int sortIndex,
								   int offset)
{
	return QUrl(QString("%1/search?"
						"query=%2&"
						"facets=%3&"
						"index=%4&"
						"offset=%5&"
						"limit=%6")
					.arg(apiBase(), ModPlatform::encodeSearchTerm(term),
						 facetList({QStringLiteral("project_type:modpack")}),
						 sortValueAt(get().sortingMethods(), sortIndex),
						 QString::number(offset),
						 QString::number(get().searchPageSize())));
}

QUrl ModrinthApi::nameSearchUrl(const QString& encodedTerm, int limit)
{
	QString url = QString("%1/search?query=%2&limit=%3")
					  .arg(apiBase(), encodedTerm, QString::number(limit));
	/* The facet's quotes are percent-encoded because this string is
	 * handed straight to QUrl with no query builder to escape them.
	 * Kept out of any arg() call: QString::arg would read "%22" as
	 * placeholder number 22. */
	url += QStringLiteral("&facets=[[%22project_type:mod%22]]");
	return QUrl(url);
}
