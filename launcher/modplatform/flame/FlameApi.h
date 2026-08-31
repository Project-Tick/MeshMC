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

#pragma once

#include <QString>
#include <QStringList>
#include <QUrl>

#include "modplatform/ContentApi.h"

/* URL builder for the CurseForge (Flame) API. */
class FlameApi final : public ModPlatform::ContentApi
{
  public:
	static const FlameApi& get();

	/* Host serving the REST API. Net::Download attaches the x-api-key
	 * header to requests for exactly this host, so the two must agree -
	 * that is why the host is published here rather than spelled out
	 * again in the networking layer. */
	static QString apiHost();
	/* Scheme + host + version prefix, no trailing slash. */
	static QString apiBase();
	/* Host serving the website. Files whose author opted out of
	 * third-party downloads have no API download URL and have to be
	 * fetched through the site instead. */
	static QString siteHost();

	/* Minecraft, in CurseForge's game list. */
	static int minecraftGameId();
	/* Section id for modpacks. Deliberately not part of ContentType:
	 * modpacks create instances rather than being installed into one. */
	static int modpackClassId();

	QString id() const override;
	QString displayName() const override;
	int searchPageSize() const override;
	/* CurseForge's search indexes neither the environment a project runs
	 * in nor its licence. */
	bool supportsExtendedFilters() const override
	{
		return false;
	}
	QList<ModPlatform::SortingMethod> sortingMethods() const override;

	QUrl searchUrl(const ModPlatform::SearchQuery& query) const override;
	QUrl projectUrl(const QString& projectId) const override;
	QUrl
	projectVersionsUrl(const ModPlatform::VersionQuery& query) const override;
	QUrl projectBodyUrl(const QString& projectId) const override;
	QUrl categoriesUrl(ModPlatform::ContentType contentType) const override;
	QUrl projectPageUrl(const QString& projectId) const override;

	/* Metadata for one specific file of one project. */
	static QUrl fileUrl(const QString& projectId, const QString& fileId);

	/* Every file of a project, with no version or loader filter. */
	static QUrl allProjectFilesUrl(const QString& projectId);

	/* Changelog for one file, as HTML wrapped in a `data` string.
	 *
	 * A request per file, unlike Modrinth which ships changelogs with
	 * the version list. That asymmetry is the reason the managed-pack
	 * page fetches changelogs lazily instead of all at once: a pack with
	 * two hundred files would otherwise mean two hundred requests to
	 * open a tab. */
	static QUrl fileChangelogUrl(const QString& projectId,
								 const QString& fileId);

	/* Modpack browsing. Separate from searchUrl() because it searches a
	 * different section and takes no version or loader filter. */
	static QUrl modpackSearchUrl(const QString& term, int sortIndex,
								 int offset);

	/* Narrow mod-name lookup, used when a dependency could not be
	 * resolved on its own platform and we go looking for it here.
	 * `term` must already be percent-encoded. */
	static QUrl nameSearchUrl(const QString& encodedTerm, int limit = 5);

	/* Browser download link for a file with no API download URL. Opening
	 * it in a browser is the sanctioned way to obtain such files. */
	static QString browserDownloadUrl(const QString& projectId,
									  const QString& fileId);
};
