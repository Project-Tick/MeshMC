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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include "ModFolderPage.h"
#include "ui_ModFolderPage.h"

#include <QMessageBox>
#include <QEvent>
#include <QKeyEvent>
#include <QAbstractItemModel>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QStandardPaths>

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/mod/ModMetadataIndex.h"
#include "ui/dialogs/BlockedModsDialog.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/DownloadContentDialog.h"
#include "ui/dialogs/DownloadSummaryDialog.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/GuiUtil.h"

#include "DesktopServices.h"

#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/PackProfile.h"
#include "minecraft/Component.h"
#include "modplatform/DependencyResolver.h"
#include "modplatform/ContentDownloadTask.h"
#include "modplatform/ContentListExport.h"
#include "modplatform/ModInstallConflictAnalyzer.h"
#include "modplatform/ModUpdateCheckTask.h"
#include "modplatform/flame/FlameApi.h"
#include "modplatform/modrinth/ModrinthApi.h"
#include "ui/dialogs/ExportListDialog.h"
#include "ui/dialogs/ModUpdateDialog.h"

#include "Version.h"

namespace
{
	// FIXME: wasteful
	void RemoveThePrefix(QString& string)
	{
		QRegularExpression regex(
			QStringLiteral("^(([Tt][Hh][eE])|([Tt][eE][Hh])) +"));
		string.remove(regex);
		string = string.trimmed();
	}
} // namespace

class ModSortProxy : public QSortFilterProxyModel
{
  public:
	explicit ModSortProxy(QObject* parent = 0) : QSortFilterProxyModel(parent)
	{
	}

  protected:
	bool filterAcceptsRow(int source_row,
						  const QModelIndex& source_parent) const override
	{
		ModFolderModel* model = qobject_cast<ModFolderModel*>(sourceModel());
		if (!model) {
			return false;
		}
		const auto& mod = model->at(source_row);
		if (mod.name().contains(filterRegularExpression())) {
			return true;
		}
		if (mod.description().contains(filterRegularExpression())) {
			return true;
		}
		for (auto& author : mod.authors()) {
			if (author.contains(filterRegularExpression())) {
				return true;
			}
		}
		return false;
	}

	bool lessThan(const QModelIndex& source_left,
				  const QModelIndex& source_right) const override
	{
		ModFolderModel* model = qobject_cast<ModFolderModel*>(sourceModel());
		if (!model || !source_left.isValid() || !source_right.isValid() ||
			source_left.column() != source_right.column()) {
			return QSortFilterProxyModel::lessThan(source_left, source_right);
		}

		// we are now guaranteed to have two valid indexes in the same column...
		// we love the provided invariants unconditionally and proceed.

		auto column = (ModFolderModel::Columns)source_left.column();
		bool invert = false;
		switch (column) {
			// GH-2550 - sort by enabled/disabled
			case ModFolderModel::ActiveColumn: {
				auto dataL = source_left.data(Qt::CheckStateRole).toBool();
				auto dataR = source_right.data(Qt::CheckStateRole).toBool();
				if (dataL != dataR) {
					return dataL > dataR;
				}
				// fallthrough
				invert = sortOrder() == Qt::DescendingOrder;
			}
			// GH-2722 - sort mod names in a way that discards "The" prefixes
			case ModFolderModel::NameColumn: {
				auto dataL =
					model
						->data(model->index(source_left.row(),
											ModFolderModel::NameColumn))
						.toString();
				RemoveThePrefix(dataL);
				auto dataR =
					model
						->data(model->index(source_right.row(),
											ModFolderModel::NameColumn))
						.toString();
				RemoveThePrefix(dataR);

				auto less = dataL.compare(dataR, sortCaseSensitivity());
				if (less != 0) {
					return invert ? (less > 0) : (less < 0);
				}
				// fallthrough
				invert = sortOrder() == Qt::DescendingOrder;
			}
			// GH-2762 - sort versions by parsing them as versions
			case ModFolderModel::VersionColumn: {
				auto dataL = Version(
					model
						->data(model->index(source_left.row(),
											ModFolderModel::VersionColumn))
						.toString());
				auto dataR = Version(
					model
						->data(model->index(source_right.row(),
											ModFolderModel::VersionColumn))
						.toString());
				return invert ? (dataL > dataR) : (dataL < dataR);
			}
			default: {
				return QSortFilterProxyModel::lessThan(source_left,
													   source_right);
			}
		}
	}
};

ModFolderPage::ModFolderPage(BaseInstance* inst,
							 std::shared_ptr<ModFolderModel> mods, QString id,
							 QString iconName, QString displayName,
							 QString helpPage, QWidget* parent)
	: QMainWindow(parent), ui(new Ui::ModFolderPage)
{
	ui->setupUi(this);
	ui->actionsToolbar->insertSpacer(ui->actionView_configs);

	m_inst = inst;
	runningStateChanged(m_inst && m_inst->isRunning());
	m_mods = mods;
	m_id = id;
	m_displayName = displayName;
	m_iconName = iconName;
	m_helpName = helpPage;
	m_fileSelectionFilter = "%1 (*.zip *.jar)";
	m_filterModel = new ModSortProxy(this);
	m_filterModel->setDynamicSortFilter(true);
	m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	m_filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	m_filterModel->setSourceModel(m_mods.get());
	m_filterModel->setFilterKeyColumn(-1);
	ui->modTreeView->setModel(m_filterModel);
	ui->modTreeView->installEventFilter(this);
	ui->modTreeView->sortByColumn(1, Qt::AscendingOrder);
	ui->modTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->modTreeView, &ModListView::customContextMenuRequested, this,
			&ModFolderPage::ShowContextMenu);
	connect(ui->modTreeView, &ModListView::activated, this,
			&ModFolderPage::modItemActivated);

	auto smodel = ui->modTreeView->selectionModel();
	connect(smodel, &QItemSelectionModel::currentChanged, this,
			&ModFolderPage::modCurrent);
	/* selectionChanged rather than currentChanged: the action is about
	 * how many rows are selected, and moving the cursor within an
	 * existing multi-row selection does not change that. */
	connect(smodel, &QItemSelectionModel::selectionChanged, this,
			&ModFolderPage::updateSelectionActions);
	connect(m_mods.get(), &ModFolderModel::updateFinished, this,
			&ModFolderPage::updateSelectionActions);
	connect(ui->filterEdit, &QLineEdit::textChanged, this,
			&ModFolderPage::on_filterTextChanged);
	connect(m_inst, &BaseInstance::runningStatusChanged, this,
			&ModFolderPage::runningStateChanged);

	/* The update button carries a menu, the way the reference launcher
	 * arranges it (ModFolderPage.cpp:84-100 there): pressing the button
	 * checks for updates, and the menu holds the two related things that
	 * would otherwise need buttons of their own.
	 *
	 * "Check for Updates" is a second entry pointing at the same action
	 * rather than the action itself: an action that owns a menu opens it
	 * on click, so the menu needs its own way of asking for the check. */
	auto* updateMenu = new QMenu(this);
	auto* checkEntry = updateMenu->addAction(tr("Check for Updates"));
	connect(checkEntry, &QAction::triggered, this,
			[this] { runUpdateCheck(); });

	/* Both go in unconditionally and are hidden when they do not apply.
	 * The content type is not known yet: subclasses call
	 * setContentType() from their own constructor, which runs after this
	 * one, so anything that depends on it has to be decided later -
	 * updateSelectionActions() does it. */
	updateMenu->addAction(ui->actionVerifyDependencies);
	updateMenu->addAction(ui->actionResetMetadata);
	ui->actionUpdate->setMenu(updateMenu);

	/* The toolbar built its buttons back in setupUi(), before the action
	 * had a menu, so it has to be told to look again. */
	ui->actionsToolbar->refreshActions();

	updateSelectionActions();
}

void ModFolderPage::modItemActivated(const QModelIndex&)
{
	if (!m_controlsEnabled) {
		return;
	}
	auto selection = m_filterModel->mapSelectionToSource(
		ui->modTreeView->selectionModel()->selection());
	m_mods->setModStatus(selection.indexes(), ModFolderModel::Toggle);
}

QMenu* ModFolderPage::createPopupMenu()
{
	QMenu* filteredMenu = QMainWindow::createPopupMenu();
	filteredMenu->removeAction(ui->actionsToolbar->toggleViewAction());
	return filteredMenu;
}

void ModFolderPage::ShowContextMenu(const QPoint& pos)
{
	auto menu = ui->actionsToolbar->createContextMenu(this, tr("Context menu"));
	menu->exec(ui->modTreeView->mapToGlobal(pos));
	delete menu;
}

void ModFolderPage::openedImpl()
{
	// startWatching() implicitly triggers the first update; subsequent
	// opens of the same page (after the user switched away and back) only
	// re-arm the QFileSystemWatcher without re-scanning, so we force an
	// update() so the list reflects anything that changed while the page
	// was closed (e.g. mods edited from another instance window).
	const bool wasWatching = m_mods->isValid() && m_mods->dir().exists();
	m_mods->startWatching();
	if (wasWatching) {
		m_mods->update();
	}
}

void ModFolderPage::closedImpl()
{
	m_mods->stopWatching();
}

void ModFolderPage::on_filterTextChanged(const QString& newContents)
{
	m_viewFilter = newContents;
	m_filterModel->setFilterFixedString(m_viewFilter);
}

CoreModFolderPage::CoreModFolderPage(BaseInstance* inst,
									 std::shared_ptr<ModFolderModel> mods,
									 QString id, QString iconName,
									 QString displayName, QString helpPage,
									 QWidget* parent)
	: ModFolderPage(inst, mods, id, iconName, displayName, helpPage, parent)
{
}

ModFolderPage::~ModFolderPage()
{
	m_mods->stopWatching();
	delete ui;
}

void ModFolderPage::runningStateChanged(bool running)
{
	if (m_controlsEnabled == !running) {
		return;
	}
	m_controlsEnabled = !running;

	// When the instance stops running, the user may have edited mods from
	// inside the game (Modrinth in-game browser, Fabric loom, etc.). The
	// QFileSystemWatcher should already have fired in most cases, but a
	// belt-and-braces refresh guarantees the list reflects the on-disk
	// reality the moment the launch session ends. We also refresh on
	// transition into running so the pre-launch state is visible.
	if (m_mods) {
		m_mods->update();
	}
	ui->actionAdd->setEnabled(m_controlsEnabled);
	ui->actionDisable->setEnabled(m_controlsEnabled);
	ui->actionEnable->setEnabled(m_controlsEnabled);
	ui->actionRemove->setEnabled(m_controlsEnabled);

	// Resource packs, shader packs and data packs can be safely added
	// while Minecraft is running (reloaded via F3+T, the settings menu or
	// /reload respectively)
	const bool canDownload = contentChangesAllowed();
	ui->actionDownload->setEnabled(canDownload);
	ui->actionAdd->setEnabled(canDownload);
	/* Update, Change version, Reset metadata, View homepage and Export
	 * list are all offered for every kind of content the page serves, as
	 * the reference launcher does (ResourcePackPage.cpp:55-71,
	 * ShaderPackPage.cpp:58-74, DataPackPage.cpp:38-54): they need
	 * nothing but a recorded origin, which downloads write for all of
	 * them. Only Verify Dependencies stays mod-only. */
	updateSelectionActions();
}

bool ModFolderPage::shouldDisplay() const
{
	return true;
}

bool CoreModFolderPage::shouldDisplay() const
{
	if (!ModFolderPage::shouldDisplay()) {
		return false;
	}

	/* Unknown answers "no", and the page appears once the answer is
	 * really known.
	 *
	 * This used to answer "yes" whenever the instance or its component
	 * list could not be inspected, which put a folder that only means
	 * something to Forge from the 1.5 era into the sidebar of instances
	 * that have nothing to do with it. The question is only answerable
	 * once the Minecraft component has been loaded and can say when that
	 * version was released, so an unloaded one is a "not yet" rather
	 * than a "sure". */
	auto* inst = dynamic_cast<MinecraftInstance*>(m_inst);
	if (!inst) {
		return false;
	}
	auto profile = inst->getPackProfile();
	if (!profile) {
		return false;
	}
	if (!profile->getComponent("net.minecraftforge")) {
		return false;
	}

	Component* minecraft = profile->getComponent("net.minecraft");
	if (!minecraft || !minecraft->m_loaded) {
		return false;
	}
	return minecraft->getReleaseDateTime() <
		   g_VersionFilterData.legacyCutoffDate;
}

bool ModFolderPage::modListFilter(QKeyEvent* keyEvent)
{
	switch (keyEvent->key()) {
		case Qt::Key_Delete:
			on_actionRemove_triggered();
			return true;
		case Qt::Key_Plus:
			on_actionAdd_triggered();
			return true;
		default:
			break;
	}
	return QWidget::eventFilter(ui->modTreeView, keyEvent);
}

bool ModFolderPage::eventFilter(QObject* obj, QEvent* ev)
{
	if (ev->type() != QEvent::KeyPress) {
		return QWidget::eventFilter(obj, ev);
	}
	QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
	if (obj == ui->modTreeView)
		return modListFilter(keyEvent);
	return QWidget::eventFilter(obj, ev);
}

void ModFolderPage::on_actionAdd_triggered()
{
	if (!contentChangesAllowed()) {
		return;
	}
	auto list = GuiUtil::BrowseForFiles(
		m_helpName,
		tr("Select %1", "Select whatever type of files the page contains. "
						"Example: 'Loader Mods'")
			.arg(m_displayName),
		m_fileSelectionFilter.arg(m_displayName),
		APPLICATION->settings()->get("CentralModsDir").toString(),
		this->parentWidget());
	if (!list.empty()) {
		for (auto filename : list) {
			m_mods->installMod(filename);
		}
	}
}

void ModFolderPage::on_actionEnable_triggered()
{
	if (!m_controlsEnabled) {
		return;
	}
	auto selection = m_filterModel->mapSelectionToSource(
		ui->modTreeView->selectionModel()->selection());
	m_mods->setModStatus(selection.indexes(), ModFolderModel::Enable);
}

void ModFolderPage::on_actionDisable_triggered()
{
	if (!m_controlsEnabled) {
		return;
	}
	auto selection = m_filterModel->mapSelectionToSource(
		ui->modTreeView->selectionModel()->selection());
	m_mods->setModStatus(selection.indexes(), ModFolderModel::Disable);
}

void ModFolderPage::on_actionRemove_triggered()
{
	if (!m_controlsEnabled) {
		return;
	}
	auto selection = m_filterModel->mapSelectionToSource(
		ui->modTreeView->selectionModel()->selection());
	m_mods->deleteMods(selection.indexes());
}

void ModFolderPage::on_actionView_configs_triggered()
{
	DesktopServices::openDirectory(m_inst->instanceConfigFolder(), true);
}

void ModFolderPage::on_actionView_Folder_triggered()
{
	DesktopServices::openDirectory(m_mods->dir().absolutePath(), true);
}

void ModFolderPage::modCurrent(const QModelIndex& current,
							   const QModelIndex& previous)
{
	if (!current.isValid()) {
		ui->frame->clear();
		return;
	}
	auto sourceCurrent = m_filterModel->mapToSource(current);
	int row = sourceCurrent.row();
	Mod& m = m_mods->operator[](row);
	ui->frame->updateWithMod(m);
}

void ModFolderPage::on_actionDownload_triggered()
{
	if (!contentChangesAllowed()) {
		return;
	}

	auto* mcInst = dynamic_cast<MinecraftInstance*>(m_inst);
	if (!mcInst) {
		return;
	}

	if (!ensureModLoaderPresent()) {
		return;
	}

	// Step 1: Open browse dialog. Hand it the persistent install index so
	// it can refuse to re-add a mod that is already installed in this
	// instance (prevents queuing a second copy/version of the same mod).
	DownloadContentDialog browseDialog(mcInst, m_contentType,
									   this->parentWidget());
	browseDialog.setInstalledIndex(m_mods->metadataIndex());
	if (browseDialog.exec() != QDialog::Accepted) {
		return;
	}

	auto selectedMods = browseDialog.selectedMods();
	if (selectedMods.isEmpty()) {
		return;
	}

	installSelection(selectedMods, browseDialog.mcVersion(),
					 browseDialog.loaderType());
}

bool ModFolderPage::ensureModLoaderPresent()
{
	if (m_contentType != ModPlatform::ContentType::Mod) {
		return true;
	}

	auto* mcInst = dynamic_cast<MinecraftInstance*>(m_inst);
	auto profile = mcInst ? mcInst->getPackProfile() : nullptr;
	bool hasLoader = false;
	if (profile) {
		hasLoader = profile->getComponent("net.minecraftforge") ||
					profile->getComponent("net.fabricmc.fabric-loader") ||
					profile->getComponent("org.quiltmc.quilt-loader") ||
					profile->getComponent("net.neoforged");
	}
	if (hasLoader) {
		return true;
	}

	QMessageBox::warning(
		this->parentWidget(), tr("No Mod Loader"),
		tr("No mod loader (Forge, Fabric, Quilt, or NeoForge) is "
		   "installed "
		   "in this instance. Please install a mod loader first before "
		   "downloading mods."));
	return false;
}

QList<int> ModFolderPage::selectedSourceRows() const
{
	QList<int> rows;
	/* The constructor reports the running state before the tree view has
	 * a model, so there is a window where there is no selection model to
	 * ask. */
	if (m_filterModel == nullptr ||
		ui->modTreeView->selectionModel() == nullptr) {
		return rows;
	}

	const auto selection = m_filterModel->mapSelectionToSource(
		ui->modTreeView->selectionModel()->selection());
	for (const auto& index : selection.indexes()) {
		if (index.isValid() && !rows.contains(index.row())) {
			rows.append(index.row());
		}
	}
	return rows;
}

int ModFolderPage::singleSelectedRow() const
{
	if (!m_mods) {
		return -1;
	}
	const auto rows = selectedSourceRows();
	if (rows.size() != 1) {
		return -1;
	}
	/* A rescan can shrink the list between the selection being made and
	 * this being asked, and ModFolderModel::at() does not tolerate a row
	 * that is no longer there. */
	const int row = rows.first();
	if (row < 0 || size_t(row) >= m_mods->size()) {
		return -1;
	}
	return row;
}

ModMetadataIndex::Entry ModFolderPage::selectedModOrigin() const
{
	const int row = singleSelectedRow();
	auto index = m_mods ? m_mods->metadataIndex() : nullptr;
	if (row < 0 || !index) {
		return ModMetadataIndex::Entry();
	}
	const Mod& mod = m_mods->at(size_t(row));
	return index->get(
		ModMetadataIndex::canonicalFileName(mod.filename().fileName()));
}

void ModFolderPage::updateChangeVersionAction()
{
	/* Reachable from runningStateChanged(), which the constructor calls
	 * before the mod list is in place. */
	if (!ui->actionChangeVersion || !m_mods) {
		return;
	}

	/* Without a recorded platform and project there is no version list
	 * to offer: the file could have come from anywhere. */
	ui->actionChangeVersion->setEnabled(
		contentChangesAllowed() && selectedModOrigin().hasPlatformOrigin());
}

QList<int> ModFolderPage::rowsForBulkAction() const
{
	QList<int> rows = selectedSourceRows();
	if (!rows.isEmpty() || !m_mods) {
		return rows;
	}
	/* Nothing selected means "all of it", which is how the update
	 * actions read to anyone who has used them. */
	for (size_t i = 0; i < m_mods->size(); ++i) {
		rows.append(int(i));
	}
	return rows;
}

QString ModFolderPage::homepageForRow(int row) const
{
	if (!m_mods || row < 0 || size_t(row) >= m_mods->size()) {
		return QString();
	}
	const Mod& mod = m_mods->at(size_t(row));

	auto index = m_mods->metadataIndex();
	if (index) {
		const ModMetadataIndex::Entry entry = index->get(
			ModMetadataIndex::canonicalFileName(mod.filename().fileName()));
		if (entry.hasPlatformOrigin()) {
			/* Each provider owns the shape of its own addresses. */
			if (entry.platform == ModrinthApi::get().id()) {
				return ModrinthApi::get()
					.projectPageUrl(entry.projectId)
					.toString();
			}
			if (entry.platform == FlameApi::get().id()) {
				return FlameApi::get()
					.projectPageUrl(entry.projectId)
					.toString();
			}
		}
	}

	/* Falls back to what the archive says about itself, so a manually
	 * added mod still has somewhere to go. */
	return mod.homeurl();
}

void ModFolderPage::updateSelectionActions()
{
	/* Reachable before the list exists: the constructor calls this and
	 * runningStateChanged() runs earlier still. */
	if (!m_mods || !ui->actionUpdate) {
		return;
	}

	updateChangeVersionAction();

	const QList<int> selected = selectedSourceRows();
	const bool hasSelection = !selected.isEmpty();
	const bool hasAnything = m_mods->size() > 0;

	/* Update and its menu apply to every kind of content: the sidecar
	 * that makes an update possible is written for resource packs,
	 * shader packs and data packs too. The reference launcher offers it
	 * on each of those pages as well (ResourcePackPage.cpp:55-67,
	 * ShaderPackPage.cpp:58-70, DataPackPage.cpp:38-50). */
	ui->actionUpdate->setEnabled(contentChangesAllowed() && hasAnything);
	ui->actionUpdate->setVisible(true);

	/* Dependencies are a mod-only idea. Decided here rather than in the
	 * constructor because that is where the content type is known. */
	ui->actionVerifyDependencies->setVisible(
		m_contentType == ModPlatform::ContentType::Mod);
	ui->actionVerifyDependencies->setEnabled(contentChangesAllowed()
											 && hasAnything);

	ui->actionResetMetadata->setEnabled(hasSelection);
	ui->actionExportList->setEnabled(hasAnything);

	/* Only offered when at least one of the selected files has anywhere
	 * to go, so the action is not a dead end. */
	bool anyHomepage = false;
	for (const int row : selected) {
		if (!homepageForRow(row).isEmpty()) {
			anyHomepage = true;
			break;
		}
	}
	ui->actionViewHomepage->setEnabled(hasSelection && anyHomepage);
}

void ModFolderPage::on_actionChangeVersion_triggered()
{
	if (!contentChangesAllowed()) {
		return;
	}

	auto* mcInst = dynamic_cast<MinecraftInstance*>(m_inst);
	if (!mcInst) {
		return;
	}

	auto metaIndex = m_mods->metadataIndex();
	if (!metaIndex) {
		return;
	}

	const int row = singleSelectedRow();
	if (row < 0) {
		return;
	}

	const Mod& mod = m_mods->at(size_t(row));
	const auto entry = selectedModOrigin();
	if (!entry.hasPlatformOrigin()) {
		QMessageBox::information(
			this->parentWidget(), tr("Cannot Change Version"),
			tr("There is no record of where '%1' came from, so its other "
			   "versions cannot be listed. Download it through this page "
			   "once and the version can be changed from then on.")
				.arg(mod.name()));
		return;
	}

	if (!ensureModLoaderPresent()) {
		return;
	}

	/* The pages would otherwise search the moment the dialog is built,
	 * only for the single-project lookup below to throw the results
	 * away. */
	DownloadContentDialog browseDialog(mcInst, m_contentType,
									   this->parentWidget(), true);
	browseDialog.setInstalledIndex(metaIndex);
	if (!browseDialog.openForVersionChange(
			entry.platform, entry.projectId,
			entry.name.isEmpty() ? mod.name() : entry.name)) {
		QMessageBox::information(
			this->parentWidget(), tr("Cannot Change Version"),
			tr("'%1' was installed from a source this page cannot browse.")
				.arg(mod.name()));
		return;
	}

	if (browseDialog.exec() != QDialog::Accepted) {
		return;
	}

	auto selectedMods = browseDialog.selectedMods();
	if (selectedMods.isEmpty()) {
		return;
	}

	/* Identical from here on: replacing a file is what the conflict
	 * analyzer makes of a pick whose project is already on disk, which
	 * is the same thing it does when the same mod is picked out of an
	 * ordinary search. */
	installSelection(selectedMods, browseDialog.mcVersion(),
					 browseDialog.loaderType());
}

void ModFolderPage::installSelection(
	const QList<ModPlatform::SelectedMod>& selectedMods,
	const QString& mcVersion, const QString& loaderType)
{
	// Step 2: Resolve dependencies (only for mods). Feed the resolver
	// the persistent install index so transitive deps that are already
	// on disk are not re-fetched and re-downloaded.
	QList<ModPlatform::DependencyInfo> dependencies;
	QList<ModPlatform::UnresolvedDep> unresolvedDeps;
	if (m_contentType == ModPlatform::ContentType::Mod) {
		/* Shared ownership, not `new` and `delete`: Skip aborts the
		 * resolver while its lookups are still unwinding, and the plain
		 * delete that used to sit at the end of this block freed the
		 * object those replies were about to land in. shared_qobject_ptr
		 * destroys through deleteLater, so the resolver outlives its own
		 * callbacks. */
		shared_qobject_ptr<DependencyResolver> resolver(
			new DependencyResolver(selectedMods, mcVersion, loaderType));
		resolver->setInstalledIndex(m_mods->metadataIndex());

		ProgressDialog depProgress(this->parentWidget());
		/* "Abort" rather than "Skip", matching the reference launcher's
		 * wording on every one of these dialogs. */
		depProgress.setSkipButton(true, tr("Abort"));
		if (depProgress.execWithTask(resolver.get()) != QDialog::Accepted) {
			if (!resolver->wasSuccessful()) {
				qWarning() << "Dependency resolution failed or was skipped";
			}
		}
		/* Whatever was resolved before Skip is still worth having. */
		dependencies = resolver->resolvedDependencies();
		unresolvedDeps = resolver->unresolvedDependencies();
	}

	reviewAndInstall(selectedMods, dependencies, unresolvedDeps);
}

void ModFolderPage::reviewAndInstall(
	const QList<ModPlatform::SelectedMod>& selectedMods,
	const QList<ModPlatform::DependencyInfo>& dependencies,
	const QList<ModPlatform::UnresolvedDep>& unresolvedDeps)
{
	// Step 3: Show summary dialog
	DownloadSummaryDialog summaryDialog(selectedMods, dependencies,
										unresolvedDeps, m_contentType,
										this->parentWidget());
	if (summaryDialog.exec() != QDialog::Accepted) {
		return;
	}

	// Step 3.5: Conflict / duplicate analysis against the on-disk index.
	// This drops items whose exact version is already installed and turns
	// items that match an existing project into in-place replacements.
	auto downloadItems = summaryDialog.downloadItems();
	auto decisions = ModInstallConflictAnalyzer::analyze(
		downloadItems, m_mods->metadataIndex());

	int alreadyInstalled = 0;
	int updates = 0;
	for (const auto& d : decisions) {
		switch (d.status) {
			case ModInstallConflictAnalyzer::Status::AlreadyInstalled:
				alreadyInstalled++;
				break;
			case ModInstallConflictAnalyzer::Status::UpdateAvailable:
			case ModInstallConflictAnalyzer::Status::NameConflict:
			case ModInstallConflictAnalyzer::Status::FileNameClash:
				updates++;
				break;
			default:
				break;
		}
	}
	if (alreadyInstalled > 0 || updates > 0) {
		qDebug() << "Mod install plan:" << alreadyInstalled
				 << "already installed," << updates << "replace,"
				 << (decisions.size() - alreadyInstalled - updates) << "fresh";
	}

	const auto plan = ModInstallConflictAnalyzer::toDownloadPlan(decisions);

	if (plan.isEmpty()) {
		QMessageBox::information(
			this->parentWidget(), tr("Nothing to do"),
			tr("Every selected mod (and its dependencies) is already "
			   "installed at the requested version."));
		return;
	}

	// Step 4: Download everything
	QString targetDir = m_mods->dir().absolutePath();

	/* Shared ownership for the same reason as the resolver above: the
	 * download can now be called off, and the transfers report back as
	 * they unwind - after the plain delete that used to sit here had
	 * already freed what they report to. */
	shared_qobject_ptr<ContentDownloadTask> downloadTask(
		new ContentDownloadTask(plan, targetDir));
	downloadTask->setMetadataIndex(m_mods->metadataIndex());
	ProgressDialog downloadProgress(this->parentWidget());
	downloadProgress.setSkipButton(true, tr("Abort"));
	downloadProgress.execWithTask(downloadTask.get());

	if (downloadTask->wasAborted()) {
		/* Nothing to complain about when the user called it off. Still
		 * rescan: some of the files will have landed before they did. */
		m_mods->update();
	} else if (downloadTask->wasSuccessful()) {
		m_mods->update();
	} else if (!offerManualDownloads(plan, targetDir)) {
		/* Not something the manual route can fix, so say what went
		 * wrong. */
		QMessageBox::warning(this->parentWidget(), tr("Download Failed"),
							 tr("Some files failed to download: %1")
								 .arg(downloadTask->failReason()));
	}

	m_mods->update();
}

bool ModFolderPage::offerManualDownloads(
	const QList<ModPlatform::DownloadItem>& plan, const QString& targetDir)
{
	/* Only files whose author blocked third-party downloads are worth
	 * offering this for. Anything else that failed - a dead connection,
	 * a checksum mismatch - is not something fetching by hand fixes, and
	 * the error message is the honest answer there. */
	QList<BlockedMod> blocked;
	const QDir dir(targetDir);
	for (const auto& item : plan) {
		if (!item.browserDownloadOnly || item.fileName.isEmpty()) {
			continue;
		}
		if (QFile::exists(dir.filePath(item.fileName))) {
			/* This one did come through after all. */
			continue;
		}

		BlockedMod entry;
		entry.projectId = item.projectId.toInt();
		entry.fileId = item.versionId.toInt();
		entry.fileName = item.fileName;
		entry.targetPath = dir.filePath(item.fileName);
		blocked.append(entry);
	}

	if (blocked.isEmpty()) {
		return false;
	}

	BlockedModsDialog dialog(
		this->parentWidget(), tr("Restricted Downloads"),
		tr("The authors of these files have blocked downloads outside the "
		   "CurseForge website, and fetching them through it did not work.\n"
		   "Click Download next to each one to open its page in your "
		   "browser. Once the files are in your Downloads folder, click "
		   "Continue and they will be moved into place."),
		blocked);

	if (dialog.exec() != QDialog::Accepted) {
		return true;
	}

	const QString downloadDir =
		QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
	int placed = 0;
	for (const auto& entry : dialog.resultMods()) {
		if (!entry.found) {
			continue;
		}
		const QString source = FS::PathCombine(downloadDir, entry.fileName);
		if (!QFile::exists(source)) {
			continue;
		}

		/* Moved rather than copied, as the pack importer does with the
		 * same dialog: a stray second copy in Downloads is how people
		 * end up installing the same jar again by hand later. A rename
		 * across volumes is not possible, hence the fallback. */
		bool ok = QFile::rename(source, entry.targetPath);
		if (!ok) {
			ok = QFile::copy(source, entry.targetPath);
		}
		if (!ok) {
			qWarning() << "Could not place manually downloaded file"
					   << entry.fileName;
			continue;
		}
		placed++;

		writeManualDownloadSidecar(plan, entry.fileName, entry.targetPath);
	}

	qDebug() << "Placed" << placed << "manually downloaded file(s)";
	return true;
}

void ModFolderPage::writeManualDownloadSidecar(
	const QList<ModPlatform::DownloadItem>& plan, const QString& fileName,
	const QString& path)
{
	auto index = m_mods->metadataIndex();
	if (!index) {
		return;
	}

	const ModPlatform::DownloadItem* source = nullptr;
	for (const auto& item : plan) {
		if (item.fileName == fileName) {
			source = &item;
			break;
		}
	}
	if (source == nullptr) {
		return;
	}

	/* A sidecar states a hash as fact, and this file came from wherever
	 * the user's browser put it - the dialog matches by name alone. If
	 * it is not the file we expected, the provenance is recorded as
	 * unknown rather than as a hash that does not hold: the update
	 * checker and the conflict analyzer both trust what is in here. */
	if (source->sha1.isEmpty()) {
		qWarning() << "No known checksum for" << fileName
				   << "- not recording where it came from";
		return;
	}

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	QCryptographicHash hash(QCryptographicHash::Sha1);
	hash.addData(&file);
	file.close();

	if (hash.result().toHex() != source->sha1.toLatin1().toLower()) {
		qWarning() << "Manually downloaded" << fileName
				   << "does not match the expected checksum - not recording "
					  "where it came from";
		return;
	}

	ModMetadataIndex::Entry entry;
	entry.fileName = source->fileName;
	entry.platform = source->platform;
	entry.projectId = source->projectId;
	entry.versionId = source->versionId;
	entry.name = source->name;
	entry.slug = source->slug;
	entry.downloadUrl = source->downloadUrl;
	entry.sha1 = source->sha1.toLower();
	entry.fileSize = source->fileSize;
	entry.isDependency = source->isDependency;
	entry.installedAt = QDateTime::currentDateTimeUtc();
	index->put(entry);
}

void ModFolderPage::verifyDependencies()
{
	if (!contentChangesAllowed() || !m_mods) {
		return;
	}
	auto index = m_mods->metadataIndex();
	if (!index) {
		return;
	}
	if (!ensureModLoaderPresent()) {
		return;
	}

	const QString mcVersion = instanceMcVersion();
	if (mcVersion.isEmpty()) {
		QMessageBox::warning(
			this->parentWidget(), tr("Cannot Verify Dependencies"),
			tr("This instance has no Minecraft version configured."));
		return;
	}

	/* The installed mods stand in for a selection: the resolver asks each
	 * provider what its recorded version requires, and drops anything
	 * already on disk at that exact version. What is left is what is
	 * missing. */
	QList<ModPlatform::SelectedMod> installed;
	for (const auto& entry : index->all()) {
		if (!entry.hasPlatformOrigin()) {
			continue;
		}
		/* Either identifier will do. A file installed from an mrpack has
		 * no version id recorded - that format lists hashes and download
		 * URLs and no version ids at all - and dropping those from the
		 * check is how "verify dependencies" came to quietly ignore every
		 * mod a modpack installed. The resolver knows how to look a
		 * version up by hash. */
		if (entry.versionId.isEmpty() && entry.sha1.isEmpty()) {
			continue;
		}
		ModPlatform::SelectedMod mod;
		mod.name = entry.name;
		mod.projectId = entry.projectId;
		mod.versionId = entry.versionId;
		mod.sha1 = entry.sha1;
		mod.slug = entry.slug;
		mod.fileName = entry.fileName;
		mod.platform = entry.platform;
		mod.mcVersion = mcVersion;
		installed.append(mod);
	}

	if (installed.isEmpty()) {
		QMessageBox::information(
			this->parentWidget(), tr("Verify Dependencies"),
			tr("None of the installed mods has a recorded origin, so there "
			   "is nothing to look up. Download mods through this page and "
			   "their dependencies can be checked from then on."));
		return;
	}

	shared_qobject_ptr<DependencyResolver> resolver(
		new DependencyResolver(installed, mcVersion, instanceLoader()));
	resolver->setInstalledIndex(index);

	ProgressDialog progress(this->parentWidget());
	progress.setSkipButton(true, tr("Abort"));
	progress.execWithTask(resolver.get());

	/* Whatever was resolved before Abort is still worth offering. */
	const auto dependencies = resolver->resolvedDependencies();
	const auto unresolved = resolver->unresolvedDependencies();

	if (dependencies.isEmpty() && unresolved.isEmpty()) {
		QMessageBox::information(
			this->parentWidget(), tr("Verify Dependencies"),
			tr("Every dependency of the installed mods is present."));
		return;
	}

	/* Deliberately not the reference launcher's arrangement: there,
	 * Verify Dependencies is the update flow with the dependency step
	 * switched on (ModFolderPage.cpp:90), so the review list also holds
	 * every available update. Here it stands on its own, which keeps the
	 * list to what the action asked about - what is missing. Updates
	 * have their own entry in the same menu. */
	reviewAndInstall({}, dependencies, unresolved);
}

void ModFolderPage::on_actionResetMetadata_triggered()
{
	if (!m_mods) {
		return;
	}
	auto index = m_mods->metadataIndex();
	if (!index) {
		return;
	}

	const QList<int> rows = selectedSourceRows();
	if (rows.isEmpty()) {
		return;
	}

	if (rows.size() > 1) {
		auto response =
			CustomMessageBox::selectable(
				this->parentWidget(), tr("Confirm Removal"),
				tr("You are about to remove the metadata for %1 files.\n"
				   "Are you sure?")
					.arg(rows.size()),
				QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No)
				->exec();
		if (response != QMessageBox::Yes) {
			return;
		}
	}

	for (const int row : rows) {
		if (row < 0 || size_t(row) >= m_mods->size()) {
			continue;
		}
		const Mod& mod = m_mods->at(size_t(row));
		index->remove(mod.filename().fileName());
	}

	m_mods->update();
	updateSelectionActions();
}

void ModFolderPage::on_actionViewHomepage_triggered()
{
	for (const int row : selectedSourceRows()) {
		const QString url = homepageForRow(row);
		if (!url.isEmpty()) {
			DesktopServices::openUrl(QUrl(url));
		}
	}
}

void ModFolderPage::on_actionExportList_triggered()
{
	if (!m_mods) {
		return;
	}

	auto index = m_mods->metadataIndex();

	QList<ContentListExport::Item> items;
	for (const int row : rowsForBulkAction()) {
		if (row < 0 || size_t(row) >= m_mods->size()) {
			continue;
		}
		const Mod& mod = m_mods->at(size_t(row));

		ContentListExport::Item item;
		item.name = mod.name();
		item.modId = mod.mmc_id();
		item.fileName = mod.filename().fileName();
		item.url = homepageForRow(row);
		item.authors = mod.authors();

		/* What the archive says about itself first, since that is the
		 * version a person recognises. Then the version number we
		 * recorded at install time, which is the same kind of string.
		 * The version id is a platform's own handle and reads like
		 * nothing at all, so it is the last resort. */
		item.version = mod.version();
		if (item.version.isEmpty() && index) {
			const auto entry =
				index->get(ModMetadataIndex::canonicalFileName(item.fileName));
			item.version = !entry.versionNumber.isEmpty() ? entry.versionNumber
														  : entry.versionId;
		}

		items.append(item);
	}

	if (items.isEmpty()) {
		return;
	}

	ExportListDialog dialog(m_inst->name(), items, this->parentWidget());
	dialog.exec();
}

QString ModFolderPage::instanceMcVersion() const
{
	auto* mcInst = dynamic_cast<MinecraftInstance*>(m_inst);
	if (!mcInst) {
		return QString();
	}
	auto profile = mcInst->getPackProfile();
	return profile ? profile->getComponentVersion("net.minecraft") : QString();
}

QString ModFolderPage::instanceLoader() const
{
	if (!ModPlatform::contentTypeUsesLoader(m_contentType)) {
		return QString();
	}
	auto* mcInst = dynamic_cast<MinecraftInstance*>(m_inst);
	if (!mcInst) {
		return QString();
	}
	auto profile = mcInst->getPackProfile();
	if (!profile) {
		return QString();
	}
	if (profile->getComponent("net.fabricmc.fabric-loader")) {
		return QStringLiteral("fabric");
	}
	if (profile->getComponent("org.quiltmc.quilt-loader")) {
		return QStringLiteral("quilt");
	}
	if (profile->getComponent("net.neoforged")) {
		return QStringLiteral("neoforge");
	}
	if (profile->getComponent("net.minecraftforge")) {
		return QStringLiteral("forge");
	}
	return QString();
}

void ModFolderPage::on_actionUpdate_triggered()
{
	runUpdateCheck();
}

void ModFolderPage::on_actionVerifyDependencies_triggered()
{
	/* Mod-only, and the action is hidden elsewhere; checked again here
	 * because a shortcut or a context menu can reach a hidden action. */
	if (m_contentType != ModPlatform::ContentType::Mod) {
		return;
	}
	verifyDependencies();
}

void ModFolderPage::runUpdateCheck()
{
	if (!contentChangesAllowed()) {
		return;
	}
	auto* mcInst = dynamic_cast<MinecraftInstance*>(m_inst);
	if (!mcInst) {
		return;
	}

	auto index = m_mods->metadataIndex();
	if (!index) {
		return;
	}

	const QString mcVersion = instanceMcVersion();
	const QString loader = instanceLoader();

	if (mcVersion.isEmpty()) {
		QMessageBox::warning(
			this->parentWidget(), tr("Cannot Check Updates"),
			tr("This instance has no Minecraft version configured."));
		return;
	}

	/* Shared ownership, and Abort now actually calls the lookups off:
	 * the button was there before but the task had no abort() to
	 * override, so pressing it did nothing at all, and the plain delete
	 * below would have freed the object its replies land in. */
	shared_qobject_ptr<ModUpdateCheckTask> check(
		new ModUpdateCheckTask(index, mcVersion, loader, m_contentType));
	ProgressDialog progress(this->parentWidget());
	progress.setSkipButton(true, tr("Abort"));
	progress.execWithTask(check.get());

	/* Whatever was checked before Abort is still worth offering. */
	const auto updates = check->availableUpdates();

	ModUpdateDialog dlg(updates, this->parentWidget());
	if (dlg.exec() != QDialog::Accepted) {
		return;
	}

	const auto plan = dlg.selectedDownloadItems();
	if (plan.isEmpty()) {
		return;
	}

	shared_qobject_ptr<ContentDownloadTask> dl(
		new ContentDownloadTask(plan, m_mods->dir().absolutePath()));
	dl->setMetadataIndex(index);
	ProgressDialog dlProgress(this->parentWidget());
	dlProgress.setSkipButton(true, tr("Abort"));
	dlProgress.execWithTask(dl.get());
	if (dl->wasAborted() || dl->wasSuccessful()) {
		m_mods->update();
	} else {
		QMessageBox::warning(
			this->parentWidget(), tr("Update Failed"),
			tr("Some updates failed to download: %1").arg(dl->failReason()));
	}
}
