/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include <QMainWindow>

#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/ModMetadataIndex.h"
#include "ui/pages/BasePage.h"
#include "modplatform/ContentType.h"
#include "modplatform/ModDownloadTypes.h"

#include <Application.h>

class ModFolderModel;
namespace Ui
{
	class ModFolderPage;
}

class ModFolderPage : public QMainWindow, public BasePage
{
	Q_OBJECT

  public:
	explicit ModFolderPage(BaseInstance* inst,
						   std::shared_ptr<ModFolderModel> mods, QString id,
						   QString iconName, QString displayName,
						   QString helpPage = "", QWidget* parent = 0);
	virtual ~ModFolderPage();

	void setFilter(const QString& filter)
	{
		m_fileSelectionFilter = filter;
	}

	void setContentType(ModPlatform::ContentType type)
	{
		m_contentType = type;
	}

	virtual QString displayName() const override
	{
		return m_displayName;
	}
	virtual QIcon icon() const override
	{
		return APPLICATION->getThemedIcon(m_iconName);
	}
	virtual QString id() const override
	{
		return m_id;
	}
	virtual QString helpPage() const override
	{
		return m_helpName;
	}
	virtual bool shouldDisplay() const override;

	virtual void openedImpl() override;
	virtual void closedImpl() override;

  protected:
	bool eventFilter(QObject* obj, QEvent* ev) override;
	bool modListFilter(QKeyEvent* ev);
	QMenu* createPopupMenu() override;

	/* Mods are the only content type Minecraft holds open while it runs.
	 * Resource packs (including legacy texture packs), shader packs and
	 * data packs can all be added mid-session and picked up on reload,
	 * so their pages stay usable while the instance is running. */
	bool allowsChangesWhileRunning() const
	{
		return m_contentType != ModPlatform::ContentType::Mod;
	}
	bool contentChangesAllowed() const
	{
		return m_controlsEnabled || allowsChangesWhileRunning();
	}

  private:
	/* Rows of the underlying model that are selected, each once.
	 * selection().indexes() hands out one index per cell, so a single
	 * selected line arrives as one index per column. */
	QList<int> selectedSourceRows() const;
	/* The one selected row, or -1 when the selection is not exactly one
	 * row that still exists. */
	int singleSelectedRow() const;
	/* Where the single selected file came from, as recorded in the
	 * folder's sidecar index. Invalid when nothing usable is selected. */
	ModMetadataIndex::Entry selectedModOrigin() const;

	/* Mods need a loader before any of this makes sense. Warns and
	 * returns false when there is none; always true for content types
	 * that do not load through one. */
	bool ensureModLoaderPresent();

	/* The instance's Minecraft version and primary loader, as the
	 * providers spell them. The loader is empty for content that does
	 * not load through one. */
	QString instanceMcVersion() const;
	QString instanceLoader() const;

	/* Check the tracked files for newer versions and offer them. */
	void runUpdateCheck();

	/* Rows the actions should act on: the selection, or everything when
	 * nothing is selected, which is how the update actions behave. */
	QList<int> rowsForBulkAction() const;

	/* Where a file's project lives on the web, from its recorded origin,
	 * falling back to whatever homepage the archive itself declares. */
	QString homepageForRow(int row) const;

	/* Everything that happens after the browse dialog has been accepted:
	 * resolve dependencies, review, work out the plan against what is on
	 * disk, download, offer the manual route if that failed.
	 *
	 * Shared by the ordinary download action and by changing one mod's
	 * version, which differ only in how the selection was arrived at. */
	void installSelection(const QList<ModPlatform::SelectedMod>& selection,
						  const QString& mcVersion, const QString& loaderType);

	/* Everything from the review dialog onwards. Split out so that the
	 * dependency check, which has no picks of its own to install, can
	 * put its findings through the same review, conflict analysis and
	 * download as an ordinary selection. */
	void reviewAndInstall(
		const QList<ModPlatform::SelectedMod>& selection,
		const QList<ModPlatform::DependencyInfo>& dependencies,
		const QList<ModPlatform::UnresolvedDep>& unresolvedDeps);

	/* Walks what is installed looking for dependencies that are not
	 * there, and offers them. */
	void verifyDependencies();

	/* Offers the manual route for files whose author blocked
	 * third-party downloads and that are still missing after a failed
	 * download. Returns whether there was anything to offer, so the
	 * caller knows whether the generic error message is still the right
	 * thing to show. */
	bool offerManualDownloads(const QList<ModPlatform::DownloadItem>& plan,
							  const QString& targetDir);
	/* Records where a manually placed file came from, but only once its
	 * checksum says it really is that file. */
	void writeManualDownloadSidecar(
		const QList<ModPlatform::DownloadItem>& plan, const QString& fileName,
		const QString& path);

  protected:
	BaseInstance* m_inst = nullptr;

  protected:
	Ui::ModFolderPage* ui = nullptr;
	std::shared_ptr<ModFolderModel> m_mods;
	QSortFilterProxyModel* m_filterModel = nullptr;
	QString m_iconName;
	QString m_id;
	QString m_displayName;
	QString m_helpName;
	QString m_fileSelectionFilter;
	QString m_viewFilter;
	bool m_controlsEnabled = true;
	ModPlatform::ContentType m_contentType = ModPlatform::ContentType::Mod;

  public slots:
	void modCurrent(const QModelIndex& current, const QModelIndex& previous);

  private slots:
	void modItemActivated(const QModelIndex& index);
	void on_filterTextChanged(const QString& newContents);
	void runningStateChanged(bool running);
	void on_actionAdd_triggered();
	void on_actionRemove_triggered();
	void on_actionEnable_triggered();
	void on_actionDisable_triggered();
	void on_actionView_Folder_triggered();
	void on_actionView_configs_triggered();
	void on_actionDownload_triggered();
	void on_actionUpdate_triggered();
	void on_actionChangeVersion_triggered();
	void on_actionVerifyDependencies_triggered();
	void on_actionResetMetadata_triggered();
	void on_actionViewHomepage_triggered();
	void on_actionExportList_triggered();
	/* Changing a version only means anything for exactly one file whose
	 * origin we recorded, so the action follows the selection. */
	void updateChangeVersionAction();
	/* The actions that only make sense for part of a selection, kept in
	 * step with it. */
	void updateSelectionActions();
	void ShowContextMenu(const QPoint& pos);
};

class CoreModFolderPage : public ModFolderPage
{
  public:
	explicit CoreModFolderPage(BaseInstance* inst,
							   std::shared_ptr<ModFolderModel> mods, QString id,
							   QString iconName, QString displayName,
							   QString helpPage = "", QWidget* parent = 0);
	virtual ~CoreModFolderPage() {}
	virtual bool shouldDisplay() const;
};
