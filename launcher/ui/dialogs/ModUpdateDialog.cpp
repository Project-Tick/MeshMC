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

#include "ModUpdateDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

ModUpdateDialog::ModUpdateDialog(
	const QList<ModUpdateCheckTask::UpdateInfo>& updates, QWidget* parent)
	: QDialog(parent), m_updates(updates)
{
	setupUi(updates);
}

void ModUpdateDialog::setupUi(
	const QList<ModUpdateCheckTask::UpdateInfo>& updates)
{
	setWindowTitle(tr("Available Mod Updates"));
	resize(620, 420);

	auto* layout = new QVBoxLayout(this);

	auto* header = new QLabel(this);
	if (updates.isEmpty()) {
		header->setText(tr("All mods are up to date."));
	} else {
		header->setText(tr("%1 mod(s) have newer compatible versions. "
						   "Tick the ones you want to update.")
							.arg(updates.size()));
	}
	header->setWordWrap(true);
	layout->addWidget(header);

	m_tree = new QTreeWidget(this);
	m_tree->setColumnCount(4);
	m_tree->setHeaderLabels(
		{tr("Mod"), tr("Source"), tr("Current"), tr("New")});
	m_tree->header()->setStretchLastSection(false);
	m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	m_tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	m_tree->setRootIsDecorated(false);
	m_tree->setUniformRowHeights(true);

	for (const auto& u : updates) {
		auto* row = new QTreeWidgetItem(m_tree);
		row->setText(0, u.name);
		row->setText(1, u.platform);
		row->setText(2, u.currentVersionId);
		row->setText(3, u.newVersionId);
		row->setToolTip(0, tr("%1 → %2\nFile: %3 → %4")
							   .arg(u.currentVersionId, u.newVersionId,
									u.currentFileName, u.item.fileName));
		row->setFlags(row->flags() | Qt::ItemIsUserCheckable);
		row->setCheckState(0, Qt::Checked);
	}
	layout->addWidget(m_tree, 1);

	auto* btnRow = new QHBoxLayout();

	auto* checkAll = new QPushButton(tr("Select all"), this);
	auto* uncheckAll = new QPushButton(tr("Select none"), this);
	connect(checkAll, &QPushButton::clicked, this, [this]() {
		for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
			m_tree->topLevelItem(i)->setCheckState(0, Qt::Checked);
		}
	});
	connect(uncheckAll, &QPushButton::clicked, this, [this]() {
		for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
			m_tree->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
		}
	});
	btnRow->addWidget(checkAll);
	btnRow->addWidget(uncheckAll);
	btnRow->addStretch(1);

	auto* buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	buttons->button(QDialogButtonBox::Ok)->setText(tr("Update"));
	buttons->button(QDialogButtonBox::Ok)->setEnabled(!updates.isEmpty());
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	btnRow->addWidget(buttons);

	layout->addLayout(btnRow);
}

QList<ModPlatform::DownloadItem> ModUpdateDialog::selectedDownloadItems() const
{
	QList<ModPlatform::DownloadItem> out;
	for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
		if (m_tree->topLevelItem(i)->checkState(0) == Qt::Checked) {
			out.append(m_updates[i].item);
		}
	}
	return out;
}
