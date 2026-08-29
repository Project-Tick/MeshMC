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
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <memory>

#include "modplatform/ContentApi.h"
#include "modplatform/ContentType.h"

class QCheckBox;
class QComboBox;
class QGroupBox;
class QVBoxLayout;
class CheckComboBox;

namespace Meta
{
	class VersionList;
}

/* What the filter panel currently asks for. */
struct ContentFilter {
	/* Minecraft versions to search for. Empty means "any version",
	 * which is how people find a mod that has not been updated yet. */
	QStringList mcVersions;
	/* Loaders to search for. Empty means no loader filter at all. */
	QStringList loaders;
	/* Which side of the game the project has to work on. Only offered
	 * on providers that index it. */
	ModPlatform::SideFilter side = ModPlatform::SideFilter::Any;
	/* Restrict to projects published under an open source licence. */
	bool openSourceOnly = false;
	/* Provider-specific category ids; empty means every category. */
	QStringList categoryIds;
	/* Release channels to offer in the version box: "release", "beta",
	 * "alpha", plus "" for a version whose channel the provider does not
	 * state - CurseForge often leaves it out. */
	QSet<QString> versionChannels;
	/* Leave out results that are already installed. */
	bool hideInstalled = false;
};

/* The panel on the left of the download dialog.
 *
 * Split into two signals on purpose: changing the Minecraft version or
 * the loaders means asking the provider again, while the release
 * channel and "hide installed" only rearrange what is already on
 * screen. Re-running a search for the latter would be a needless round
 * trip and would lose the user's place in the list. */
class ContentFilterWidget final : public QWidget
{
	Q_OBJECT

  public:
	explicit ContentFilterWidget(QWidget* parent = nullptr);

	/* Builds the groups that make sense for this kind of content, and
	 * ticks the instance's own version and loader. Call once, after the
	 * owning page has been set up.
	 *
	 * `extended` turns on the groups only one provider can answer -
	 * environment and licence. The reference launcher does the same:
	 * they are shown on the Modrinth page and hidden on CurseForge,
	 * whose search indexes neither, so an offered box would be a
	 * control that quietly does nothing. */
	void setup(ModPlatform::ContentType contentType, const QString& mcVersion,
			   const QString& instanceLoader, bool extended);

	const ContentFilter& filter() const
	{
		return m_filter;
	}

	/* Whether a version on this release channel may be offered. Always
	 * true until setup() has run: a panel that was never built has no
	 * boxes, and an empty channel set must not be read as "allow
	 * nothing" on the pages that do without filtering. */
	bool allowsVersionChannel(const QString& channel) const;

	/* The part of the filter the provider has to be asked about. The
	 * instance's own Minecraft version is remembered by setup(), so
	 * this needs nothing passed in. */
	ModPlatform::SearchFilters searchFilters() const;

	/* Fills the (initially empty) category group once the provider has
	 * answered. Safe to call before or after the panel is first shown. */
	void setCategories(const QList<ModPlatform::Category>& categories);

  signals:
	/* The provider has to be asked again. */
	void searchFilterChanged();
	/* Only what is already loaded needs re-examining. */
	void viewFilterChanged();

  private:
	QCheckBox* addChannelBox(QGroupBox* group, const QString& label,
							 const QString& channel);
	void buildVersionGroup(bool extended);
	/* Asks the metadata index for Minecraft's version list. Answers by
	 * filling the version box whenever the list turns up. */
	void loadMcVersions();
	/* (Re)fills the version box from the loaded list, honouring "show
	 * all versions" and keeping whatever the user had picked. */
	void applyMcVersions();
	void rebuildFilter();

  private:
	ContentFilter m_filter;
	/* Set by setup(). Until then the panel has no boxes and its filter
	 * means nothing. */
	bool m_active = false;

	/* Inside the scroll area; every group is added to this rather than
	 * to the widget's own layout. */
	QVBoxLayout* m_contentLayout = nullptr;

	/* The instance's own version, ticked by default. Kept because the
	 * version box may be filled long after setup() ran, and because the
	 * search filters are built from it before the list arrives. */
	QString m_instanceMcVersion;
	/* Several versions at once, on providers that can search for them.
	 * Null on the others, which get the single combo below instead. */
	CheckComboBox* m_mcVersionsBox = nullptr;
	/* One version, plus an "All Versions" entry. Null when the box
	 * above is in use. */
	QComboBox* m_mcVersionBox = nullptr;
	/* Off by default, so the list is releases only - the same reason
	 * the version page defaults that way. */
	QCheckBox* m_showAllVersionsBox = nullptr;
	/* Held so the list can be refiltered when "show all versions" is
	 * toggled without asking the network again. */
	std::shared_ptr<Meta::VersionList> m_mcVersionList;

	QCheckBox* m_hideInstalledBox = nullptr;
	QCheckBox* m_clientSideBox = nullptr;
	QCheckBox* m_serverSideBox = nullptr;
	QCheckBox* m_openSourceBox = nullptr;
	/* Empty until the provider answers; kept so the boxes can be added
	 * to it later. Hidden while it has nothing in it. */
	QGroupBox* m_categoryGroup = nullptr;
	/* Loader checkbox -> the name the providers use. */
	QList<QPair<QCheckBox*, QString>> m_loaderBoxes;
	/* Channel checkbox -> "release" / "beta" / "alpha" / "". */
	QList<QPair<QCheckBox*, QString>> m_channelBoxes;
	/* Category checkbox -> provider-specific id. */
	QList<QPair<QCheckBox*, QString>> m_categoryBoxes;
};
