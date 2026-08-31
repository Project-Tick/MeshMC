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

#include <QIcon>
#include <QModelIndex>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QWidget>

#include "modplatform/ContentProviderModel.h"
#include "ui/pages/BasePage.h"

namespace Ui
{
	class ContentProviderPage;
}

class DownloadContentDialog;
class ModMetadataIndex;
class ProgressWidget;

/* One provider's tab inside the download dialog: search bar, result
 * list, description pane and version picker.
 *
 * The page owns no download state of its own. Whether a project is
 * queued is asked of the dialog, which keeps the single queue shared by
 * every provider - the same mod must not be queued twice just because
 * it exists on both sites. */
class ContentProviderPage final : public QWidget, public BasePage
{
	Q_OBJECT

  public:
	ContentProviderPage(DownloadContentDialog* dialog,
						ContentProviderModel* model, QIcon icon);
	~ContentProviderPage() override;

	QString id() const override;
	QString displayName() const override;
	QIcon icon() const override;
	bool shouldDisplay() const override
	{
		return true;
	}
	void openedImpl() override;

	QString searchTerm() const;
	/* Carries the term over when the user switches provider, so the two
	 * tabs behave like one search bar. */
	void setSearchTerm(const QString& term);

	/* Look a project up by its slug and select it once the results are
	 * in. Used when a project link is pasted or followed. */
	void openProjectSlug(const QString& slug);

	/* Turns the page into a version picker for one project: the search
	 * bar, the sort box, the filter panel and the result list all go
	 * away, and an Ok/Cancel pair appears with Ok reading "Reinstall".
	 *
	 * Used when the version of something already installed is being
	 * changed, where browsing is beside the point - the project is
	 * already decided and only the version is in question. */
	void openProject(const QString& projectId);

	/* Skip the search the page would otherwise run the first time it is
	 * shown. The page is opened by its container as soon as the dialog is
	 * built, which for a version change means a search nobody asked for
	 * that is thrown away moments later by openProject(). Consumed
	 * once. */
	void setSuppressInitialSearch(bool suppress);

	/* Called by the dialog after the queue changed on any page.
	 * `queuedNames` are normalized project names. */
	void queueChanged(const QSet<QString>& queuedNames);
	void setInstalledIndex(std::shared_ptr<ModMetadataIndex> index);

	bool eventFilter(QObject* watched, QEvent* event) override;

  private slots:
	void triggerSearch();
	void onSearchTextEdited();
	void onSortChanged(int index);
	void onCurrentChanged(const QModelIndex& current,
						  const QModelIndex& previous);
	void onVersionChanged(int index);
	void onSelectionButtonClicked();
	void onToggleRequested(const QModelIndex& index);
	void onEntryUpdated(int row);
	void onSearchStateChanged();
	void onAnchorClicked(const QUrl& url);
	/* The provider has to be asked again: version or loaders changed. */
	void onSearchFilterChanged();
	/* Only what is already loaded needs re-examining. */
	void onViewFilterChanged();

  private:
	const ModPlatform::IndexedProject* currentProject() const;
	int currentRow() const;

	/* Whether the release channel filter lets this version be offered. */
	bool versionAllowed(const ModPlatform::ContentVersion& version) const;
	/* Newest version the channel filter still allows, or -1 when the
	 * filter hides every one of them. */
	int firstAllowedVersionIndex(
		const ModPlatform::IndexedProject& project) const;

	void updateDescription();
	void updateVersionList();
	void updateSelectionButton();

	/* Index into the current project's versions, or -1. */
	int selectedVersionIndex() const;
	void queueProject(int row, int versionIndex);

	/* Selects the row matching a slug we were asked to jump to, if the
	 * results contain it. */
	void selectPendingSlug();
	/* Selects the first row of a single-project lookup, or says the
	 * project is gone. One-shot, armed by openProject(). */
	void selectLookedUpProject();

  private:
	Ui::ContentProviderPage* m_ui;
	DownloadContentDialog* m_dialog;
	ContentProviderModel* m_model;
	QIcon m_icon;

	ProgressWidget* m_searchProgress;
	QTimer m_searchTimer;

	/* Rows the user ticked before their versions had arrived. They are
	 * queued as soon as the version list lands. */
	QSet<int> m_pendingToggles;

	/* What the version box currently holds, as "<project>/<count>".
	 * Rebuilding it unconditionally would throw away the version the
	 * user picked every time another part of the row turns up. */
	QString m_versionListKey;
	/* Likewise for the description, whose rebuild also scrolls back to
	 * the top and restarts image loading. */
	QString m_descriptionKey;

	/* Slug of a project we were told to open, waiting for the search
	 * that will contain it. */
	QString m_pendingSlug;

	/* See setSuppressInitialSearch(). */
	bool m_suppressInitialSearch = false;
	/* Version-change mode. Following a link out of the description would
	 * take the user to a different project, which is not what this
	 * window is for any more, so links go to the web browser instead. */
	bool m_versionChangeMode = false;
	/* Armed by openProject() until the lookup lands. */
	bool m_awaitingLookedUpProject = false;
};
