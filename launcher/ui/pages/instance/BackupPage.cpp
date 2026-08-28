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

#include "BackupPage.h"
#include "ui_BackupPage.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QTreeWidgetItem>

#include "backup/BackupTask.h"
#include "ui/dialogs/ProgressDialog.h"

BackupPage::BackupPage(BaseInstance* inst, QWidget* parent)
	: QWidget(parent), ui(new Ui::BackupPage), m_inst(inst)
{
	ui->setupUi(this);

	m_manager = std::make_unique<BackupManager>(m_inst->id(),
												m_inst->instanceRoot());

	ui->backupList->header()->setStretchLastSection(false);
	ui->backupList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	ui->backupList->header()->setSectionResizeMode(
		1, QHeaderView::ResizeToContents);
	ui->backupList->header()->setSectionResizeMode(
		2, QHeaderView::ResizeToContents);

	connect(ui->backupList, &QTreeWidget::itemSelectionChanged, this,
			&BackupPage::onSelectionChanged);

	refreshList();
}

BackupPage::~BackupPage()
{
	delete ui;
}

void BackupPage::openedImpl()
{
	refreshList();
}

void BackupPage::refreshList()
{
	ui->backupList->clear();
	m_entries = m_manager->listBackups();

	for (const auto& entry : m_entries) {
		auto* item = new QTreeWidgetItem(ui->backupList);
		item->setText(0, entry.name.isEmpty() ? entry.fileName : entry.name);
		item->setText(1, entry.timestamp.toString(
							 QStringLiteral("yyyy-MM-dd HH:mm:ss")));
		item->setText(2, humanFileSize(entry.sizeBytes));
		item->setData(0, Qt::UserRole, entry.fullPath);
	}

	const int count = m_entries.size();
	ui->statusLabel->setText(tr("%n backup(s)", "", count));
	updateButtons();
}

void BackupPage::updateButtons()
{
	const bool hasSelection = !ui->backupList->selectedItems().isEmpty();
	ui->btnRestore->setEnabled(hasSelection);
	ui->btnExport->setEnabled(hasSelection);
	ui->btnDelete->setEnabled(hasSelection);
}

void BackupPage::onSelectionChanged()
{
	updateButtons();
}

BackupEntry BackupPage::selectedEntry() const
{
	const auto items = ui->backupList->selectedItems();
	if (items.isEmpty())
		return {};

	const QString path = items.first()->data(0, Qt::UserRole).toString();
	for (const auto& e : m_entries) {
		if (e.fullPath == path)
			return e;
	}
	return {};
}

void BackupPage::on_btnCreate_clicked()
{
	bool ok = false;
	const QString label = QInputDialog::getText(
		this, tr("Create Backup"), tr("Backup label (optional):"),
		QLineEdit::Normal, {}, &ok);

	if (!ok)
		return;

	// Off to a worker thread with a progress bar in front of it — an
	// instance can be gigabytes, and this used to be the freeze that
	// made the whole window stop repainting.
	BackupTask task(m_inst->id(), m_inst->instanceRoot(), label);
	ProgressDialog progress(this);
	progress.execWithTask(&task);

	if (task.wasSuccessful()) {
		QMessageBox::information(
			this, tr("Backup Created"),
			tr("Backup created successfully:\n%1").arg(task.result().fileName));
	} else {
		QMessageBox::warning(this, tr("Backup Failed"),
							 tr("Failed to create backup:\n%1")
								 .arg(task.failReason()));
	}

	refreshList();
}

void BackupPage::on_btnRestore_clicked()
{
	const auto entry = selectedEntry();
	if (!entry.isValid())
		return;

	// Restoring wipes the instance directory. Doing that under a running
	// game means half-deleted worlds and mods the JVM still has open.
	if (m_inst->isRunning()) {
		QMessageBox::warning(
			this, tr("Instance Running"),
			tr("This instance is currently running. Close the game before "
			   "restoring a backup."));
		return;
	}

	const auto ret = QMessageBox::warning(
		this, tr("Restore Backup"),
		tr("This will replace the current instance contents with the "
		   "backup:\n\n"
		   "%1\n\n"
		   "This action cannot be undone. Continue?")
			.arg(entry.fileName),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if (ret != QMessageBox::Yes)
		return;

	ui->statusLabel->setText(tr("Restoring backup..."));
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	if (m_manager->restoreBackup(entry)) {
		QMessageBox::information(this, tr("Restore Complete"),
								 tr("Backup restored successfully."));
	} else {
		QMessageBox::warning(
			this, tr("Restore Failed"),
			tr("Failed to restore backup. Check the logs for details."));
	}

	refreshList();
}

void BackupPage::on_btnExport_clicked()
{
	const auto entry = selectedEntry();
	if (!entry.isValid())
		return;

	const QString dest = QFileDialog::getSaveFileName(
		this, tr("Export Backup"), entry.fileName, tr("Zip Files (*.zip)"));

	if (dest.isEmpty())
		return;

	if (m_manager->exportBackup(entry, dest)) {
		QMessageBox::information(this, tr("Export Complete"),
								 tr("Backup exported to:\n%1").arg(dest));
	} else {
		QMessageBox::warning(this, tr("Export Failed"),
							 tr("Failed to export backup."));
	}
}

void BackupPage::on_btnImport_clicked()
{
	const QString src = QFileDialog::getOpenFileName(
		this, tr("Import Backup"), {}, tr("Zip Files (*.zip)"));

	if (src.isEmpty())
		return;

	bool ok = false;
	const QString label = QInputDialog::getText(
		this, tr("Import Backup"), tr("Label for imported backup (optional):"),
		QLineEdit::Normal, {}, &ok);

	if (!ok)
		return;

	const auto entry = m_manager->importBackup(src, label);
	if (!entry.isValid()) {
		QMessageBox::warning(this, tr("Import Failed"),
							 tr("Failed to import backup."));
	} else {
		QMessageBox::information(this, tr("Import Complete"),
								 tr("Backup imported successfully."));
	}

	refreshList();
}

void BackupPage::on_btnDelete_clicked()
{
	const auto entry = selectedEntry();
	if (!entry.isValid())
		return;

	const auto ret = QMessageBox::question(
		this, tr("Delete Backup"),
		tr("Delete the selected backup?\n\n%1\n\nThis cannot be undone.")
			.arg(entry.fileName),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if (ret != QMessageBox::Yes)
		return;

	m_manager->deleteBackup(entry);
	refreshList();
}

/* static */
QString BackupPage::humanFileSize(qint64 bytes)
{
	if (bytes < 1024)
		return QStringLiteral("%1 B").arg(bytes);
	if (bytes < 1024 * 1024)
		return QStringLiteral("%1 KiB").arg(bytes / 1024.0, 0, 'f', 1);
	if (bytes < 1024LL * 1024 * 1024)
		return QStringLiteral("%1 MiB").arg(bytes / (1024.0 * 1024.0), 0, 'f',
										   1);
	return QStringLiteral("%1 GiB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0,
										'f', 2);
}
