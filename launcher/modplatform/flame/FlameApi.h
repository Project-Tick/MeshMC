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
