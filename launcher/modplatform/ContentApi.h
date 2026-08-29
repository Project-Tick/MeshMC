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

#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "modplatform/ContentType.h"

/* One place that knows how to address CurseForge and Modrinth.
 *
 * Before this existed the two base URLs were spelled out at roughly
 * thirty call sites across the dependency resolver, the update checker,
 * the search models and the download dialog, each with its own slightly
 * different idea of when to attach a loader filter. Anything that talks
 * to either platform should now go through the matching ContentApi so
 * that a change to an endpoint - or to the rule for a query parameter -
 * happens exactly once.
 *
 * These are pure URL builders: no networking, no state, cheap to call.
 * Issuing the request and parsing the reply stays with the caller,
 * because the two platforms return quite different JSON shapes and
 * pretending otherwise would only push the difference somewhere less
 * obvious. */
namespace ModPlatform
{

	/* One entry of a provider's sort menu.
	 *
	 * The two providers do not offer the same sorts, in the same order,
	 * or even the same number of them: CurseForge has eight numbered
	 * fields, Modrinth five named ones. A single hard-coded list in the
	 * dialog therefore cannot work - it silently asked CurseForge to
	 * sort by author when the user picked "follows". Each provider now
	 * publishes its own list and the UI rebuilds the combo box from it. */
	struct SortingMethod {
		/* Value the provider's own API expects. */
		QString apiValue;
		/* Label shown in the sort combo box. */
		QString readableName;
	};

	/* Which side of the game a project has to work on.
	 *
	 * Only Modrinth indexes this. CurseForge's search has no equivalent
	 * field and ignores the setting, which is why the filter panel only
	 * offers it on the Modrinth page. */
	enum class SideFilter { Any, Client, Server, Universal };

	/* One entry of a provider's category list.
	 *
	 * `id` is whatever that provider wants back in a search: a number on
	 * CurseForge, the name itself on Modrinth. It is never shown, so the
	 * difference stays inside the two API classes. */
	struct Category {
		QString name;
		QString id;
	};

	/* The half of the filter panel that changes what the provider is
	 * asked for, as opposed to what is done with the answer.
	 *
	 * Kept together in one struct because it travels from the panel
	 * through the page into the model, and adding a filter should not
	 * mean widening three signatures on the way. */
	struct SearchFilters {
		/* Empty means every Minecraft version, which is what the panel
		 * sends when nothing is ticked in its version box.
		 *
		 * A list because a search over several versions at once is a
		 * reasonable thing to want - a mod that stopped at 1.20.1 is
		 * still worth seeing next to one built for 1.20.4. Only
		 * Modrinth can express it; see each searchUrl(). */
		QStringList mcVersions;
		/* Empty means "any loader" rather than "no loader". Ignored for
		 * content types that are not loader-specific - see
		 * contentTypeUsesLoader(). */
		QStringList loaders;
		SideFilter side = SideFilter::Any;
		bool openSourceOnly = false;
		/* Provider-specific category ids, as handed out by
		 * ContentApi::categoriesUrl(). Empty means every category. */
		QStringList categoryIds;

		bool operator==(const SearchFilters& other) const
		{
			return mcVersions == other.mcVersions &&
				   loaders == other.loaders && side == other.side &&
				   openSourceOnly == other.openSourceOnly &&
				   categoryIds == other.categoryIds;
		}
		bool operator!=(const SearchFilters& other) const
		{
			return !(*this == other);
		}
	};

	/* What to search for. */
	struct SearchQuery {
		ContentType contentType = ContentType::Mod;
		QString term;
		SearchFilters filters;
		/* Index into this provider's sortingMethods(). Out-of-range
		 * values fall back to the first entry. */
		int sortIndex = 0;
		int offset = 0;
	};

	/* Which versions of a project to list. Empty filters are omitted
	 * from the query string rather than sent empty.
	 *
	 * `mcVersions` and `loaders` are lists for the same reason
	 * SearchQuery's are: the filter panel lets several be ticked at
	 * once. The two providers disagree on what to do with more than one
	 * of either - see each projectVersionsUrl() - so this hands them the
	 * whole list and lets them decide. */
	struct VersionQuery {
		QString projectId;
		ContentType contentType = ContentType::Mod;
		QStringList mcVersions;
		QStringList loaders;
	};

	/* One loader name as a query list. An empty name yields an empty
	 * list, which means "no loader filter" rather than "a project with
	 * no loader" - the callers that only ever know about a single
	 * loader, such as the dependency resolver and the update checker,
	 * go through this rather than each spelling the check out. */
	inline QStringList singleLoaderList(const QString& loader)
	{
		return loader.isEmpty() ? QStringList() : QStringList{loader};
	}

	/* The same for a single Minecraft version. Kept separate from
	 * singleLoaderList() so the call reads as what it is at the call
	 * site, where the two sit next to each other. */
	inline QStringList singleVersionList(const QString& version)
	{
		return version.isEmpty() ? QStringList() : QStringList{version};
	}

	/* A user-typed search term, safe to drop into a query string.
	 *
	 * Both providers take the term as a query parameter, so an
	 * unescaped "&" ends that parameter early and everything after it is
	 * read as another one: searching for "carry on & tweaks" quietly
	 * asked for something else entirely. Spaces, "+" and "#" have the
	 * same problem. */
	inline QString encodeSearchTerm(const QString& term)
	{
		return QString::fromLatin1(QUrl::toPercentEncoding(term));
	}

	class ContentApi
	{
	  public:
		virtual ~ContentApi() = default;

		/* Stable identifier, matching the `platform` field stored in
		 * ModMetadataIndex sidecars and SelectedMod/DownloadItem:
		 * "curseforge" or "modrinth". */
		virtual QString id() const = 0;
		/* Name shown to the user. */
		virtual QString displayName() const = 0;
		/* How many results the provider returns per search page. */
		virtual int searchPageSize() const = 0;

		/* Whether this provider's search indexes what SearchFilters
		 * calls `side` and `openSourceOnly`. Only Modrinth does. The
		 * filter panel asks so that it can leave those groups out
		 * rather than offer boxes that quietly change nothing, which is
		 * what the reference launcher does too. */
		virtual bool supportsExtendedFilters() const = 0;

		/* The sorts this provider understands, in the order the UI
		 * should offer them. Never empty. */
		virtual QList<SortingMethod> sortingMethods() const = 0;

		virtual QUrl searchUrl(const SearchQuery& query) const = 0;
		virtual QUrl projectUrl(const QString& projectId) const = 0;
		virtual QUrl projectVersionsUrl(const VersionQuery& query) const = 0;

		/* Long-form project description, as HTML. The two providers
		 * disagree on the format they hand out - one returns HTML, the
		 * other Markdown - so converting is part of parsing the reply,
		 * not the caller's problem. */
		virtual QUrl projectBodyUrl(const QString& projectId) const = 0;

		/* The categories this provider offers for a kind of content.
		 * Fetched once when the filter panel is built; the reply is
		 * turned into Category entries by the model, since the two
		 * providers answer with quite different JSON. */
		virtual QUrl categoriesUrl(ContentType contentType) const = 0;

		/* The project's page on the provider's website, for opening in a
		 * browser. Not an API endpoint: this is the address a person is
		 * meant to read, which is why it is asked of the provider rather
		 * than assembled by the caller from a template. */
		virtual QUrl projectPageUrl(const QString& projectId) const = 0;
	};

} // namespace ModPlatform
