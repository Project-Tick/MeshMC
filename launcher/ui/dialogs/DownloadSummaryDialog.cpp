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
 */

#include "DownloadSummaryDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>

DownloadSummaryDialog::DownloadSummaryDialog(
	const QList<ModPlatform::SelectedMod>& selectedMods,
	const QList<ModPlatform::DependencyInfo>& dependencies,
	const QList<ModPlatform::UnresolvedDep>& unresolvedDeps, QWidget* parent)
	: QDialog(parent), m_selectedMods(selectedMods),
	  m_dependencies(dependencies), m_unresolvedDeps(unresolvedDeps)
{
	// Build download items list
	for (const auto& mod : m_selectedMods) {
		ModPlatform::DownloadItem item;
		item.name = mod.name;
		item.fileName = mod.fileName;
		item.downloadUrl = mod.downloadUrl;
		item.sha1 = mod.sha1;
		item.fileSize = mod.fileSize;
		item.isDependency = false;
		m_downloadItems.append(item);
	}
	for (const auto& dep : m_dependencies) {
		ModPlatform::DownloadItem item;
		item.name = dep.name;
		item.fileName = dep.fileName;
		item.downloadUrl = dep.downloadUrl;
		item.sha1 = dep.sha1;
		item.fileSize = dep.fileSize;
		item.isDependency = true;
		m_downloadItems.append(item);
	}

	setupUi();
}

void DownloadSummaryDialog::setupUi()
{
	setWindowTitle(tr("Download Summary"));
	resize(600, 450);

	auto* layout = new QVBoxLayout(this);

	m_summaryLabel = new QLabel(this);
	int totalFiles = m_downloadItems.size();
	int depCount = m_dependencies.size();
	int modCount = m_selectedMods.size();
	if (m_unresolvedDeps.isEmpty()) {
		m_summaryLabel->setText(
			tr("Ready to download %1 file(s): %2 selected + %3 dependency(ies)")
				.arg(totalFiles)
				.arg(modCount)
				.arg(depCount));
	} else {
		m_summaryLabel->setText(tr("Ready to download %1 file(s): %2 selected "
								   "+ %3 dependency(ies)\n"
								   "⚠ %4 dependency(ies) could NOT be resolved "
								   "(see warnings below)")
									.arg(totalFiles)
									.arg(modCount)
									.arg(depCount)
									.arg(m_unresolvedDeps.size()));
		m_summaryLabel->setStyleSheet("QLabel { color: #cc6600; }");
	}
	m_summaryLabel->setWordWrap(true);
	layout->addWidget(m_summaryLabel);

	m_treeWidget = new QTreeWidget(this);
	m_treeWidget->setHeaderLabels(
		{tr("Name"), tr("File"), tr("Type"), tr("Source")});
	m_treeWidget->header()->setStretchLastSection(true);
	m_treeWidget->setRootIsDecorated(false);
	m_treeWidget->setAlternatingRowColors(true);

	// Add selected mods
	for (const auto& mod : m_selectedMods) {
		auto* item = new QTreeWidgetItem(m_treeWidget);
		item->setText(0, mod.name);
		item->setText(1, mod.fileName);
		item->setText(2, tr("Selected"));
		item->setText(3, mod.platform == "curseforge" ? tr("CurseForge")
													  : tr("Modrinth"));
	}

	// Add dependencies
	for (const auto& dep : m_dependencies) {
		auto* item = new QTreeWidgetItem(m_treeWidget);
		item->setText(0, dep.name);
		item->setText(1, dep.fileName);
		item->setText(2, tr("Dependency"));
		item->setText(3, dep.platform == "curseforge" ? tr("CurseForge")
													  : tr("Modrinth"));
		item->setForeground(2, QBrush(Qt::darkGreen));
	}

	// Add unresolved dependencies as warnings
	for (const auto& unresolved : m_unresolvedDeps) {
		auto* item = new QTreeWidgetItem(m_treeWidget);
		item->setText(0, unresolved.name.isEmpty() ? unresolved.projectId
												   : unresolved.name);
		item->setText(1, tr("NOT FOUND"));
		item->setText(2, tr("⚠ Unresolved"));
		item->setText(3, tr("No compatible version"));
		item->setForeground(0, QBrush(Qt::red));
		item->setForeground(1, QBrush(Qt::red));
		item->setForeground(2, QBrush(Qt::red));
		item->setForeground(3, QBrush(Qt::red));
	}

	m_treeWidget->resizeColumnToContents(0);
	m_treeWidget->resizeColumnToContents(1);
	m_treeWidget->resizeColumnToContents(2);
	layout->addWidget(m_treeWidget, 1);

	// Buttons
	auto* buttonLayout = new QHBoxLayout();
	buttonLayout->addStretch();

	m_cancelButton = new QPushButton(tr("Cancel"), this);
	m_continueButton = new QPushButton(tr("Continue"), this);
	m_continueButton->setDefault(true);

	connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
	connect(m_continueButton, &QPushButton::clicked, this, &QDialog::accept);

	buttonLayout->addWidget(m_cancelButton);
	buttonLayout->addWidget(m_continueButton);
	layout->addLayout(buttonLayout);
}
