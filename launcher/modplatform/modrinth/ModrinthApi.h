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

/* URL builder for the Modrinth API. */
class ModrinthApi final : public ModPlatform::ContentApi
{
  public:
	static const ModrinthApi& get();

	static QString apiHost();
	/* Scheme + host + version prefix, no trailing slash. */
	static QString apiBase();

	QString id() const override;
	QString displayName() const override;
	int searchPageSize() const override;
	bool supportsExtendedFilters() const override
	{
		return true;
	}
	QList<ModPlatform::SortingMethod> sortingMethods() const override;

	QUrl searchUrl(const ModPlatform::SearchQuery& query) const override;
	QUrl projectUrl(const QString& projectId) const override;
	QUrl
	projectVersionsUrl(const ModPlatform::VersionQuery& query) const override;
	QUrl projectBodyUrl(const QString& projectId) const override;
	QUrl categoriesUrl(ModPlatform::ContentType contentType) const override;
	QUrl projectPageUrl(const QString& projectId) const override;

	/* A single version addressed by its own id. Modrinth version ids are
	 * globally unique, so unlike CurseForge this needs no project id. */
	static QUrl versionUrl(const QString& versionId);

	/* Whether @p candidate can be a Modrinth version or project id.
	 *
	 * They are base62 and eight characters long, which is exactly what a
	 * version *number* is not: numbers carry dots, plus signs and loader
	 * names ("1.1.1+1.17"). The distinction matters because the two are
	 * easy to confuse - a file's CDN URL spells out the number, not the
	 * id - and asking the version endpoint for a number gets a 404 that
	 * looks like a dependency with nothing to resolve. */
	static bool isVersionId(const QString& candidate);

	/* The version that owns a file with the given SHA-1.
	 *
	 * The way back to a version when all we kept was the file: an mrpack
	 * manifest lists hashes and download URLs and no version ids at all,
	 * so for everything installed from one this is the only honest
	 * lookup. Answers with the same version object as versionUrl(). */
	static QUrl versionByHashUrl(const QString& sha1);

	/* Project versions restricted to an explicit set of loaders. Used
	 * for modpack browsing, where any of the mod loaders will do. */
	static QUrl projectVersionsUrlForLoaders(const QString& projectId,
											 const QStringList& loaders);

	/* Modpack browsing. Separate from searchUrl() because modpacks are
	 * not a ContentType - they create instances rather than being
	 * installed into one - and take no version or loader facet. */
	static QUrl modpackSearchUrl(const QString& term, int sortIndex,
								 int offset);

	/* Narrow mod-name lookup, used when a dependency could not be
	 * resolved on its own platform and we go looking for it here.
	 * `encodedTerm` must already be percent-encoded. */
	static QUrl nameSearchUrl(const QString& encodedTerm, int limit = 5);
};
