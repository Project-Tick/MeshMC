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

#include "ModUpdateDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <KLocalizedString>

ModUpdateDialog::ModUpdateDialog(
	const QList<ModUpdateCheckTask::UpdateInfo>& updates, QWidget* parent)
	: QDialog(parent), m_updates(updates)
{
	setupUi(updates);
}

void ModUpdateDialog::setupUi(
	const QList<ModUpdateCheckTask::UpdateInfo>& updates)
{
	setWindowTitle(i18n("Available Mod Updates"));
	resize(620, 420);

	auto* layout = new QVBoxLayout(this);

	auto* header = new QLabel(this);
	if (updates.isEmpty()) {
		header->setText(i18n("All mods are up to date."));
	} else {
		header->setText(i18n("%1 mod(s) have newer compatible versions. "
						   "Tick the ones you want to update.", updates.size()));
	}
	header->setWordWrap(true);
	layout->addWidget(header);

	m_tree = new QTreeWidget(this);
	m_tree->setColumnCount(4);
	m_tree->setHeaderLabels(
		{i18n("Mod"), i18n("Source"), i18n("Current"), i18n("New")});
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
		row->setToolTip(0, i18n("%1 → %2\nFile: %3 → %4", u.currentVersionId, u.newVersionId, u.currentFileName, u.item.fileName));
		row->setFlags(row->flags() | Qt::ItemIsUserCheckable);
		row->setCheckState(0, Qt::Checked);
	}
	layout->addWidget(m_tree, 1);

	auto* btnRow = new QHBoxLayout();

	auto* checkAll = new QPushButton(i18n("Select all"), this);
	auto* uncheckAll = new QPushButton(i18n("Select none"), this);
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
	buttons->button(QDialogButtonBox::Ok)->setText(i18n("Update"));
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
