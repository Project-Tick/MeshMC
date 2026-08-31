/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Project Tick
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FlameApi.h"

#include <QObject>

namespace
{

	/* The sortField for a UI index, clamped to the published list. */
	QString sortValueAt(const QList<ModPlatform::SortingMethod>& sorts,
						int sortIndex)
	{
		if (sortIndex >= 0 && sortIndex < sorts.size()) {
			return sorts.at(sortIndex).apiValue;
		}
		return sorts.first().apiValue;
	}

	/* Appends `param` to `url`, choosing '?' or '&' as appropriate. */
	void appendParam(QString& url, const QString& param)
	{
		url += (url.contains(QLatin1Char('?')) ? QLatin1Char('&')
											   : QLatin1Char('?'));
		url += param;
	}

} // namespace

const FlameApi& FlameApi::get()
{
	static const FlameApi instance;
	return instance;
}

QString FlameApi::apiHost()
{
	return QStringLiteral("api.curseforge.com");
}

QString FlameApi::apiBase()
{
	return QStringLiteral("https://") + apiHost() + QStringLiteral("/v1");
}

QString FlameApi::siteHost()
{
	return QStringLiteral("www.curseforge.com");
}

int FlameApi::minecraftGameId()
{
	return 432;
}

int FlameApi::modpackClassId()
{
	return 4471;
}

QString FlameApi::id() const
{
	return QStringLiteral("curseforge");
}

QString FlameApi::displayName() const
{
	return QStringLiteral("CurseForge");
}

int FlameApi::searchPageSize() const
{
	return 25;
}

QList<ModPlatform::SortingMethod> FlameApi::sortingMethods() const
{
	/* Values are CurseForge's ModsSearchSortField enum. They are sent
	 * verbatim as sortField, so the order of this list is free to be
	 * whatever reads best - it is no longer tied to the wire format. */
	return {
		{QStringLiteral("1"), QObject::tr("Sort by Featured")},
		{QStringLiteral("2"), QObject::tr("Sort by Popularity")},
		{QStringLiteral("3"), QObject::tr("Sort by Last Updated")},
		{QStringLiteral("4"), QObject::tr("Sort by Name")},
		{QStringLiteral("5"), QObject::tr("Sort by Author")},
		{QStringLiteral("6"), QObject::tr("Sort by Downloads")},
		{QStringLiteral("7"), QObject::tr("Sort by Category")},
		{QStringLiteral("8"), QObject::tr("Sort by Game Version")},
	};
}

QUrl FlameApi::searchUrl(const ModPlatform::SearchQuery& query) const
{
	/* One multi-arg call rather than a chain: the search term is
	 * user-supplied, and a chained arg() rescans the string it has
	 * already built, so a term containing "%7" would be mistaken for the
	 * next placeholder. The multi-arg form substitutes in a single pass
	 * and never looks at what it inserted. */
	QString url = QString("%1/mods/search?"
						  "gameId=%2&"
						  "classId=%3&"
						  "index=%4&"
						  "pageSize=%5&"
						  "searchFilter=%6&"
						  "sortField=%7&"
						  "sortOrder=desc")
					  .arg(apiBase(), QString::number(minecraftGameId()),
						   QString::number(
							   ModPlatform::contentTypeToCurseForgeClassId(
								   query.contentType)),
						   QString::number(query.offset),
						   QString::number(searchPageSize()),
						   ModPlatform::encodeSearchTerm(query.term),
						   sortValueAt(sortingMethods(), query.sortIndex));

	if (!query.filters.mcVersions.isEmpty()) {
		/* CurseForge's search takes one game version, so a filter that
		 * names several can only be honoured for the first of them -
		 * which is what the reference launcher sends too. Narrowing to
		 * the newest would be a guess about which one the user meant;
		 * the panel offers the list in the order the provider will see
		 * it. */
		url += QStringLiteral("&gameVersion=") +
			   query.filters.mcVersions.first();
	}

	if (!query.filters.loaders.isEmpty() &&
		ModPlatform::contentTypeUsesLoader(query.contentType)) {
		/* Numeric ModLoaderType ids, not names. A loader CurseForge has
		 * no id for - the panel offers several that only Modrinth
		 * indexes - is dropped rather than sent through, which is what
		 * the reference launcher does as well.
		 *
		 * When that leaves nothing the parameter is omitted altogether:
		 * an empty bracket pair is not something the API promises to
		 * accept, and "no loader filter" is the honest reading of a
		 * filter this provider cannot express. */
		QStringList loaderIds;
		loaderIds.reserve(query.filters.loaders.size());
		for (const QString& loader : query.filters.loaders) {
			const int id = ModPlatform::loaderToCurseForgeModLoaderType(loader);
			if (id > 0) {
				loaderIds.append(QString::number(id));
			}
		}
		if (!loaderIds.isEmpty()) {
			url += QStringLiteral("&modLoaderTypes=[") +
				   loaderIds.join(QLatin1Char(',')) + QStringLiteral("]");
		}
	}

	if (!query.filters.categoryIds.isEmpty()) {
		/* Bare numeric ids, unquoted - CurseForge rejects the quoted
		 * form the loader list uses. */
		url += QStringLiteral("&categoryIds=[") +
			   query.filters.categoryIds.join(QLatin1Char(',')) +
			   QStringLiteral("]");
	}

	/* SearchFilters::side and ::openSourceOnly are deliberately not used
	 * here: CurseForge's search indexes neither, so there is nothing to
	 * send. The panel hides both boxes on this provider's page. */

	return QUrl(url);
}

QUrl FlameApi::projectUrl(const QString& projectId) const
{
	return QUrl(QString("%1/mods/%2").arg(apiBase(), projectId));
}

QUrl FlameApi::projectBodyUrl(const QString& projectId) const
{
	/* CurseForge serves the long description as ready-made HTML on its
	 * own endpoint, rather than as a field of the project. */
	return QUrl(QString("%1/mods/%2/description").arg(apiBase(), projectId));
}

QUrl FlameApi::categoriesUrl(ModPlatform::ContentType contentType) const
{
	return QUrl(
		QString("%1/categories?gameId=%2&classId=%3")
			.arg(apiBase(), QString::number(minecraftGameId()),
				 QString::number(ModPlatform::contentTypeToCurseForgeClassId(
					 contentType))));
}

QUrl FlameApi::projectPageUrl(const QString& projectId) const
{
	/* Not under the API host and deliberately not a per-content-type
	 * path: the site resolves a numeric project id to whatever kind of
	 * project it is and redirects, which saves us from having to know. */
	return QUrl(QStringLiteral("https://www.curseforge.com/projects/") +
				projectId);
}

QUrl FlameApi::projectVersionsUrl(const ModPlatform::VersionQuery& query) const
{
	/* The whole list in one go, as the reference launcher asks for. The
	 * default page is fifty files, which for a long-lived mod is a
	 * window onto some arbitrary part of its history - and the callers
	 * that pick "the newest one that fits" can only be right if they are
	 * looking at all of them. */
	QString url = QString("%1/mods/%2/files?pageSize=10000")
					  .arg(apiBase(), query.projectId);

	if (!query.mcVersions.isEmpty()) {
		/* One game version only, as in the search above. */
		appendParam(url, QStringLiteral("gameVersion=") +
							 query.mcVersions.first());
	}

	/* The file list takes a single modLoaderType, so it can only be sent
	 * when exactly one loader is wanted. With several ticked the filter
	 * is dropped and every file is listed, which is what the reference
	 * launcher does as well: narrowing to an arbitrary one of them would
	 * hide versions the user asked to see. */
	if (query.loaders.size() == 1 &&
		ModPlatform::contentTypeUsesLoader(query.contentType)) {
		const int loaderType =
			ModPlatform::loaderToCurseForgeModLoaderType(query.loaders.first());
		if (loaderType > 0) {
			appendParam(url, QStringLiteral("modLoaderType=") +
								 QString::number(loaderType));
		}
	}

	return QUrl(url);
}

QUrl FlameApi::fileUrl(const QString& projectId, const QString& fileId)
{
	return QUrl(QString("%1/mods/%2/files/%3").arg(apiBase(), projectId,
												   fileId));
}

QUrl FlameApi::allProjectFilesUrl(const QString& projectId)
{
	return QUrl(QString("%1/mods/%2/files").arg(apiBase(), projectId));
}

QUrl FlameApi::fileChangelogUrl(const QString& projectId, const QString& fileId)
{
	return QUrl(QString("%1/mods/%2/files/%3/changelog")
					.arg(apiBase(), projectId, fileId));
}

QUrl FlameApi::modpackSearchUrl(const QString& term, int sortIndex, int offset)
{
	return QUrl(QString("%1/mods/search?"
						"gameId=%2&"
						"classId=%3&"
						"index=%4&"
						"pageSize=%5&"
						"searchFilter=%6&"
						"sortField=%7&"
						"sortOrder=desc")
					.arg(apiBase(), QString::number(minecraftGameId()),
						 QString::number(modpackClassId()),
						 QString::number(offset),
						 QString::number(get().searchPageSize()),
						 ModPlatform::encodeSearchTerm(term),
						 sortValueAt(get().sortingMethods(), sortIndex)));
}

QUrl FlameApi::nameSearchUrl(const QString& encodedTerm, int limit)
{
	return QUrl(QString("%1/mods/search?"
						"gameId=%2&"
						"classId=%3&"
						"searchFilter=%4&"
						"sortField=2&"
						"sortOrder=desc&"
						"pageSize=%5")
					.arg(apiBase(), QString::number(minecraftGameId()),
						 QString::number(
							 ModPlatform::contentTypeToCurseForgeClassId(
								 ModPlatform::ContentType::Mod)),
						 encodedTerm, QString::number(limit)));
}

QString FlameApi::browserDownloadUrl(const QString& projectId,
									 const QString& fileId)
{
	return QString("https://%1/api/v1/mods/%2/files/%3/download")
		.arg(siteHost(), projectId, fileId);
}
