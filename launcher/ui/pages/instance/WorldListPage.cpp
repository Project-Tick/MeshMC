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
 * Copyright 2015-2021 MultiMC Contributors
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

#include "WorldListPage.h"
#include "ui_WorldListPage.h"
#include "minecraft/WorldList.h"

#include <QEvent>
#include <QMenu>
#include <QKeyEvent>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeView>
#include <QInputDialog>
#include <QProcess>
#include <QVBoxLayout>

#include <algorithm>

#include "tools/MCEditTool.h"
#include "FileSystem.h"

#include "minecraft/mod/DataPackFolderModel.h"
#include "ui/GuiUtil.h"
#include "ui/pages/BasePageProvider.h"
#include "ui/pages/instance/DataPackPage.h"
#include "ui/widgets/PageContainer.h"
#include "DesktopServices.h"

#include "Application.h"

class WorldListProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

  public:
	WorldListProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

	virtual QVariant data(const QModelIndex& index,
						  int role = Qt::DisplayRole) const
	{
		QModelIndex sourceIndex = mapToSource(index);

		if (index.column() == 0 && role == Qt::DecorationRole) {
			WorldList* worlds = qobject_cast<WorldList*>(sourceModel());
			auto iconFile =
				worlds->data(sourceIndex, WorldList::IconFileRole).toString();
			if (iconFile.isNull()) {
				// NOTE: Minecraft uses the same placeholder for servers AND
				// worlds
				return APPLICATION->getThemedIcon("unknown_server");
			}
			return QIcon(iconFile);
		}

		return sourceIndex.data(role);
	}
};

WorldListPage::WorldListPage(BaseInstance* inst,
							 std::shared_ptr<WorldList> worlds, QWidget* parent)
	: QMainWindow(parent), m_inst(inst), ui(new Ui::WorldListPage),
	  m_worlds(worlds)
{
	ui->setupUi(this);

	ui->toolBar->insertSpacer(ui->actionRefresh);

	WorldListProxyModel* proxy = new WorldListProxyModel(this);
	proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
	proxy->setSourceModel(m_worlds.get());
	ui->worldTreeView->setSortingEnabled(true);
	ui->worldTreeView->setModel(proxy);
	ui->worldTreeView->installEventFilter(this);
	ui->worldTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->worldTreeView->setIconSize(QSize(64, 64));
	connect(ui->worldTreeView, &QTreeView::customContextMenuRequested, this,
			&WorldListPage::ShowContextMenu);

	auto head = ui->worldTreeView->header();
	head->setSectionResizeMode(0, QHeaderView::Stretch);
	head->setSectionResizeMode(1, QHeaderView::ResizeToContents);

	connect(ui->worldTreeView->selectionModel(),
			&QItemSelectionModel::currentChanged, this,
			&WorldListPage::worldChanged);
	worldChanged(QModelIndex(), QModelIndex());
}

void WorldListPage::openedImpl()
{
	m_worlds->startWatching();
}

void WorldListPage::closedImpl()
{
	m_worlds->stopWatching();
}

WorldListPage::~WorldListPage()
{
	m_worlds->stopWatching();
	delete ui;
}

void WorldListPage::ShowContextMenu(const QPoint& pos)
{
	auto menu = ui->toolBar->createContextMenu(this, tr("Context menu"));
	menu->exec(ui->worldTreeView->mapToGlobal(pos));
	delete menu;
}

QMenu* WorldListPage::createPopupMenu()
{
	QMenu* filteredMenu = QMainWindow::createPopupMenu();
	filteredMenu->removeAction(ui->toolBar->toggleViewAction());
	return filteredMenu;
}

bool WorldListPage::shouldDisplay() const
{
	return true;
}

bool WorldListPage::worldListFilter(QKeyEvent* keyEvent)
{
	switch (keyEvent->key()) {
		case Qt::Key_Delete:
			on_actionRemove_triggered();
			return true;
		default:
			break;
	}
	return QWidget::eventFilter(ui->worldTreeView, keyEvent);
}

bool WorldListPage::eventFilter(QObject* obj, QEvent* ev)
{
	if (ev->type() != QEvent::KeyPress) {
		return QWidget::eventFilter(obj, ev);
	}
	QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
	if (obj == ui->worldTreeView)
		return worldListFilter(keyEvent);
	return QWidget::eventFilter(obj, ev);
}

void WorldListPage::on_actionRemove_triggered()
{
	auto proxiedIndex = getSelectedWorld();

	if (!proxiedIndex.isValid())
		return;

	auto result = QMessageBox::question(
		this, tr("Are you sure?"),
		tr("This will remove the selected world permenantly.\n"
		   "The world will be gone forever (A LONG TIME).\n"
		   "\n"
		   "Do you want to continue?"));
	if (result != QMessageBox::Yes) {
		return;
	}
	m_worlds->stopWatching();
	m_worlds->deleteWorld(proxiedIndex.row());
	m_worlds->startWatching();
}

void WorldListPage::on_actionView_Folder_triggered()
{
	DesktopServices::openDirectory(m_worlds->dir().absolutePath(), true);
}

void WorldListPage::on_actionDatapacks_triggered()
{
	QModelIndex index = getSelectedWorld();

	if (!index.isValid()) {
		return;
	}

	if (!worldSafetyNagQuestion())
		return;

	const QString fullPath =
		m_worlds->data(index, WorldList::FolderRole).toString();
	const QString folder = FS::PathCombine(fullPath, "datapacks");
	const QString worldName =
		m_worlds->data(index, WorldList::NameRole).toString();

	/* This page also backs legacy instances, which have no pack profile
	 * to search against and predate data packs entirely. Keep the old
	 * "just open the folder" behaviour for them. */
	auto* mcInst = dynamic_cast<MinecraftInstance*>(m_inst);
	if (!mcInst) {
		DesktopServices::openDirectory(folder, true);
		return;
	}

	/* Vanilla loads data packs per world, so this is the folder that
	 * actually matters. Previously we just opened it in the file manager;
	 * now it gets the same list/add/remove/download page the other
	 * content folders have, hosted in its own modal dialog. */
	auto* dialog = new QDialog(this);
	dialog->setWindowTitle(tr("Data packs for %1").arg(worldName));
	dialog->setWindowModality(Qt::WindowModal);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->resize(
		static_cast<int>(std::max(0.5 * window()->width(), 400.0)),
		static_cast<int>(std::max(0.75 * window()->height(), 400.0)));
	dialog->restoreGeometry(QByteArray::fromBase64(
		APPLICATION->settings()->get("DataPackManagerGeometry").toByteArray()));

	m_datapackModel.reset(new DataPackFolderModel(folder));

	/* The provider is only read while PageContainer is being built, so a
	 * stack object outlives its use even though the dialog is modeless. */
	GenericPageProvider provider(dialog->windowTitle());
	provider.addPageCreator([this, mcInst]() -> BasePage* {
		return new DataPackPage(mcInst, m_datapackModel);
	});

	auto* layout = new QVBoxLayout(dialog);

	// The page's own widgets would otherwise grab the default button and
	// swallow Enter; park it on a hidden button instead.
	auto* focusStealer = new QPushButton(dialog);
	focusStealer->setDefault(true);
	focusStealer->hide();
	layout->addWidget(focusStealer);

	auto* pageContainer = new PageContainer(&provider, {}, dialog);
	pageContainer->hidePageList();
	layout->addWidget(pageContainer);

	auto* buttonBox =
		new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Help);
	connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
	connect(buttonBox, &QDialogButtonBox::helpRequested, pageContainer,
			&PageContainer::help);
	layout->addWidget(buttonBox);

	connect(dialog, &QDialog::finished, this, [this, dialog] {
		APPLICATION->settings()->set("DataPackManagerGeometry",
									 dialog->saveGeometry().toBase64());
		// The dialog only borrowed the model; drop it so the folder is
		// no longer watched once the manager is gone.
		m_datapackModel.reset();
	});

	dialog->open();
}

void WorldListPage::on_actionReset_Icon_triggered()
{
	auto proxiedIndex = getSelectedWorld();

	if (!proxiedIndex.isValid())
		return;

	if (m_worlds->resetIcon(proxiedIndex.row())) {
		ui->actionReset_Icon->setEnabled(false);
	}
}

QModelIndex WorldListPage::getSelectedWorld()
{
	auto index = ui->worldTreeView->selectionModel()->currentIndex();

	auto proxy = (QSortFilterProxyModel*)ui->worldTreeView->model();
	return proxy->mapToSource(index);
}

void WorldListPage::on_actionCopy_Seed_triggered()
{
	QModelIndex index = getSelectedWorld();

	if (!index.isValid()) {
		return;
	}
	int64_t seed = m_worlds->data(index, WorldList::SeedRole).toLongLong();
	APPLICATION->clipboard()->setText(QString::number(seed));
}

void WorldListPage::on_actionMCEdit_triggered()
{
	if (m_mceditStarting)
		return;

	auto mcedit = APPLICATION->mcedit();

	const QString mceditPath = mcedit->path();

	QModelIndex index = getSelectedWorld();

	if (!index.isValid()) {
		return;
	}

	if (!worldSafetyNagQuestion())
		return;

	auto fullPath = m_worlds->data(index, WorldList::FolderRole).toString();

	auto program = mcedit->getProgramPath();
	if (program.size()) {
#ifdef Q_OS_WIN32
		if (!QProcess::startDetached(program, {fullPath}, mceditPath)) {
			mceditError();
		}
#else
		m_mceditProcess.reset(new LoggedProcess());
		m_mceditProcess->setDetachable(true);
		connect(m_mceditProcess.get(), &LoggedProcess::stateChanged, this,
				&WorldListPage::mceditState);
		m_mceditProcess->start(program, {fullPath});
		m_mceditProcess->setWorkingDirectory(mceditPath);
		m_mceditStarting = true;
#endif
	} else {
		QMessageBox::warning(
			this->parentWidget(), tr("No MCEdit found or set up!"),
			tr("You do not have MCEdit set up or it was moved.\nYou can set it "
			   "up in the global settings."));
	}
}

void WorldListPage::mceditError()
{
	QMessageBox::warning(
		this->parentWidget(), tr("MCEdit failed to start!"),
		tr("MCEdit failed to start.\nIt may be necessary to reinstall it."));
}

void WorldListPage::mceditState(LoggedProcess::State state)
{
	bool failed = false;
	switch (state) {
		case LoggedProcess::NotRunning:
		case LoggedProcess::Starting:
			return;
		case LoggedProcess::FailedToStart:
		case LoggedProcess::Crashed:
		case LoggedProcess::Aborted: {
			failed = true;
		}
		case LoggedProcess::Running:
		case LoggedProcess::Finished: {
			m_mceditStarting = false;
			break;
		}
	}
	if (failed) {
		mceditError();
	}
}

void WorldListPage::worldChanged(const QModelIndex& current,
								 const QModelIndex& previous)
{
	QModelIndex index = getSelectedWorld();
	bool enable = index.isValid();
	ui->actionCopy_Seed->setEnabled(enable);
	ui->actionMCEdit->setEnabled(enable);
	ui->actionRemove->setEnabled(enable);
	ui->actionCopy->setEnabled(enable);
	ui->actionRename->setEnabled(enable);
	ui->actionDatapacks->setEnabled(enable);
	bool hasIcon = !index.data(WorldList::IconFileRole).isNull();
	ui->actionReset_Icon->setEnabled(enable && hasIcon);
}

void WorldListPage::on_actionAdd_triggered()
{
	auto list = GuiUtil::BrowseForFiles(displayName(),
										tr("Select a Minecraft world zip"),
										tr("Minecraft World Zip File (*.zip)"),
										QString(), this->parentWidget());
	if (!list.empty()) {
		m_worlds->stopWatching();
		for (auto filename : list) {
			m_worlds->installWorld(QFileInfo(filename));
		}
		m_worlds->startWatching();
	}
}

bool WorldListPage::isWorldSafe(QModelIndex)
{
	return !m_inst->isRunning();
}

bool WorldListPage::worldSafetyNagQuestion()
{
	if (!isWorldSafe(getSelectedWorld())) {
		auto result = QMessageBox::question(
			this, tr("Copy World"),
			tr("Changing a world while Minecraft is running is potentially "
			   "unsafe.\nDo you wish to proceed?"));
		if (result == QMessageBox::No) {
			return false;
		}
	}
	return true;
}

void WorldListPage::on_actionCopy_triggered()
{
	QModelIndex index = getSelectedWorld();
	if (!index.isValid()) {
		return;
	}

	if (!worldSafetyNagQuestion())
		return;

	auto worldVariant = m_worlds->data(index, WorldList::ObjectRole);
	auto world = (World*)worldVariant.value<void*>();
	bool ok = false;
	QString name = QInputDialog::getText(this, tr("World name"),
										 tr("Enter a new name for the copy."),
										 QLineEdit::Normal, world->name(), &ok);

	if (ok && name.length() > 0) {
		world->install(m_worlds->dir().absolutePath(), name);
	}
}

void WorldListPage::on_actionRename_triggered()
{
	QModelIndex index = getSelectedWorld();
	if (!index.isValid()) {
		return;
	}

	if (!worldSafetyNagQuestion())
		return;

	auto worldVariant = m_worlds->data(index, WorldList::ObjectRole);
	auto world = (World*)worldVariant.value<void*>();

	bool ok = false;
	QString name = QInputDialog::getText(this, tr("World name"),
										 tr("Enter a new world name."),
										 QLineEdit::Normal, world->name(), &ok);

	if (ok && name.length() > 0) {
		world->rename(name);
	}
}

void WorldListPage::on_actionRefresh_triggered()
{
	m_worlds->update();
}

#include "WorldListPage.moc"
