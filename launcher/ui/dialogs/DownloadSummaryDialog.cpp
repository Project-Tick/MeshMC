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

#include "DownloadSummaryDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QShortcut>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <algorithm>

namespace
{

	/* The provider's name as people know it, from the id stored in
	 * sidecars and download items. */
	QString providerName(const QString& platform)
	{
		if (platform == QStringLiteral("curseforge")) {
			return QStringLiteral("CurseForge");
		}
		if (platform == QStringLiteral("modrinth")) {
			return QStringLiteral("Modrinth");
		}
		return platform;
	}

} // namespace

DownloadSummaryDialog::DownloadSummaryDialog(
	const QList<ModPlatform::SelectedMod>& selectedMods,
	const QList<ModPlatform::DependencyInfo>& dependencies,
	const QList<ModPlatform::UnresolvedDep>& unresolvedDeps,
	ModPlatform::ContentType contentType, QWidget* parent)
	: QDialog(parent), m_selectedMods(selectedMods),
	  m_dependencies(dependencies), m_unresolvedDeps(unresolvedDeps),
	  m_contentType(contentType)
{
	setupUi();

	/* Both lists are sorted by name before being shown, as in the
	 * reference launcher: the order they happened to resolve in says
	 * nothing useful to the person reading them. */
	auto byName = [](const QString& a, const QString& b) {
		return QString::compare(a, b, Qt::CaseInsensitive) < 0;
	};

	/* Provenance fields are carried over so the downloader can write a
	 * sidecar pinning each file to its remote origin, which is what the
	 * update and conflict pipelines read on later runs. */
	auto sortedMods = m_selectedMods;
	std::sort(sortedMods.begin(), sortedMods.end(),
			  [&byName](const ModPlatform::SelectedMod& a,
						const ModPlatform::SelectedMod& b) {
				  return byName(a.name, b.name);
			  });

	for (const auto& mod : sortedMods) {
		ModPlatform::DownloadItem item;
		item.name = mod.name;
		item.fileName = mod.fileName;
		item.downloadUrl = mod.downloadUrl;
		item.sha1 = mod.sha1;
		item.fileSize = mod.fileSize;
		item.isDependency = false;
		item.platform = mod.platform;
		item.projectId = mod.projectId;
		item.versionId = mod.versionId;
		item.slug = mod.slug;
		item.browserDownloadOnly = mod.browserDownloadOnly;

		appendRow(item, providerName(mod.platform), {}, mod.versionType,
				  /*isDependency=*/false, /*enabled=*/true);
	}

	auto sortedDeps = m_dependencies;
	std::sort(sortedDeps.begin(), sortedDeps.end(),
			  [&byName](const ModPlatform::DependencyInfo& a,
						const ModPlatform::DependencyInfo& b) {
				  return byName(a.name, b.name);
			  });

	for (const auto& dep : sortedDeps) {
		ModPlatform::DownloadItem item;
		item.name = dep.name;
		item.fileName = dep.fileName;
		item.downloadUrl = dep.downloadUrl;
		item.sha1 = dep.sha1;
		item.fileSize = dep.fileSize;
		item.isDependency = true;
		item.platform = dep.platform;
		item.projectId = dep.projectId;
		item.versionId = dep.versionId;
		item.slug = dep.slug;
		item.browserDownloadOnly = dep.browserDownloadOnly;

		/* Something already on disk at another version starts unticked:
		 * it is usually there for a reason, and replacing it should be a
		 * decision rather than a side effect. */
		appendRow(item, providerName(dep.platform), dep.requiredBy,
				  dep.versionType, /*isDependency=*/true,
				  /*enabled=*/!dep.maybeInstalled);
	}

	/* Not from the reference launcher, which has no cross-provider
	 * lookup and so nothing to report as unresolved. Listed without a
	 * tick box, since there is nothing here to download. */
	for (const auto& unresolved : m_unresolvedDeps) {
		auto* item = new QTreeWidgetItem(m_rootItem);
		item->setText(0, unresolved.name.isEmpty() ? unresolved.projectId
												   : unresolved.name);
		item->setText(1, tr("not found"));
		item->setFlags(Qt::ItemIsEnabled);
		item->setForeground(0, QBrush(Qt::red));
		item->setForeground(1, QBrush(Qt::red));
		item->setToolTip(
			0, tr("No compatible version of this dependency could be found "
				  "on either provider. The %1 that needs it may not work.")
				   .arg(ModPlatform::contentTypeNoun(m_contentType)));
	}

	m_rootItem->setExpanded(true);
	m_treeWidget->resizeColumnToContents(0);
}

void DownloadSummaryDialog::appendRow(const ModPlatform::DownloadItem& item,
									  const QString& provider,
									  const QStringList& requiredBy,
									  const QString& versionType,
									  bool isDependency, bool enabled)
{
	auto* row = new QTreeWidgetItem(m_rootItem);
	row->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
				  Qt::ItemIsSelectable);
	row->setCheckState(0, enabled ? Qt::Checked : Qt::Unchecked);
	row->setText(0, item.name);
	if (!enabled) {
		row->setToolTip(0, tr("Unticked because a version of this is already "
							  "installed. Tick it to replace that version."));
	}

	/* Details go on child rows rather than into columns: a file name
	 * alone is often wider than the whole dialog. The bare value is kept
	 * under UserRole so copying a line yields the value rather than the
	 * label along with it. */
	auto* fileNameItem = new QTreeWidgetItem(row);
	fileNameItem->setText(0, tr("Filename: %1").arg(item.fileName));
	fileNameItem->setData(0, Qt::UserRole, item.fileName);

	auto* providerItem = new QTreeWidgetItem(row);
	providerItem->setText(0, tr("Provider: %1").arg(provider));
	providerItem->setData(0, Qt::UserRole, provider);

	if (!requiredBy.isEmpty()) {
		auto* requiredByItem = new QTreeWidgetItem(row);
		if (requiredBy.size() == 1) {
			requiredByItem->setText(
				0, tr("Required by: %1").arg(requiredBy.back()));
			requiredByItem->setData(0, Qt::UserRole, requiredBy.back());
		} else {
			requiredByItem->setText(0, tr("Required by:"));
			for (const QString& name : requiredBy) {
				auto* nameItem = new QTreeWidgetItem(requiredByItem);
				nameItem->setText(0, name);
			}
		}
	}

	if (!versionType.isEmpty()) {
		auto* versionTypeItem = new QTreeWidgetItem(row);
		versionTypeItem->setText(0, tr("Version type: %1").arg(versionType));
		versionTypeItem->setData(0, Qt::UserRole, versionType);
	}

	if (item.browserDownloadOnly) {
		/* Said out loud before the attempt, because this is the one kind
		 * of download that can be refused outright - and if it is, the
		 * file has to be fetched by hand afterwards. */
		auto* blockedItem = new QTreeWidgetItem(row);
		blockedItem->setText(
			0, tr("The author has blocked third-party downloads for this "
				  "file. It will be fetched through the website instead, "
				  "which does not always work."));
		blockedItem->setForeground(0, QBrush(Qt::darkYellow));
	}

	if (isDependency) {
		m_dependencyItems.append(row);
		/* Only worth offering once there is something to toggle. */
		m_toggleDepsButton->setVisible(true);
	}

	row->setExpanded(true);

	m_rows.append({row, item});
}

QList<ModPlatform::DownloadItem> DownloadSummaryDialog::downloadItems() const
{
	QList<ModPlatform::DownloadItem> items;
	for (const auto& row : m_rows) {
		if (row.item != nullptr && row.item->checkState(0) == Qt::Checked) {
			items.append(row.download);
		}
	}
	return items;
}

void DownloadSummaryDialog::onToggleDependencies()
{
	m_dependenciesChecked = !m_dependenciesChecked;
	const auto state = m_dependenciesChecked ? Qt::Checked : Qt::Unchecked;
	for (auto* item : m_dependencyItems) {
		item->setCheckState(0, state);
	}
}

void DownloadSummaryDialog::setupUi()
{
	const QString contents = ModPlatform::contentTypeNounPlural(m_contentType);

	setWindowTitle(tr("Confirm %1 selection").arg(contents));
	setSizeGripEnabled(true);
	setModal(true);
	resize(500, 350);

	auto* layout = new QVBoxLayout(this);

	m_explainLabel = new QLabel(
		tr("You're about to download the following %1:").arg(contents), this);
	m_explainLabel->setWordWrap(true);
	layout->addWidget(m_explainLabel);

	m_treeWidget = new QTreeWidget(this);
	m_treeWidget->setColumnCount(2);
	m_treeWidget->setHeaderHidden(true);
	m_treeWidget->setAlternatingRowColors(true);
	/* Nothing here is opened or acted on, so a selection would only be
	 * visual noise - but individual cells still have to be reachable for
	 * the copy shortcut below. */
	m_treeWidget->setSelectionMode(QAbstractItemView::NoSelection);
	m_treeWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
	m_treeWidget->header()->setStretchLastSection(false);
	m_treeWidget->header()->setSectionResizeMode(
		1, QHeaderView::ResizeToContents);
	layout->addWidget(m_treeWidget, 1);

	/* One root above the rows, tri-state, so it doubles as tick-all and
	 * shows at a glance whether everything is going to be downloaded. */
	m_rootItem = new QTreeWidgetItem(m_treeWidget);
	m_rootItem->setText(0, contents);
	m_rootItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
						 Qt::ItemIsSelectable | Qt::ItemIsAutoTristate);
	m_rootItem->setCheckState(0, Qt::Checked);
	m_rootItem->setExpanded(true);

	/* Collapsing the root would hide the whole list, which is never what
	 * someone wants here; it folds the rows' details away instead. */
	connect(m_treeWidget, &QTreeWidget::itemCollapsed, this,
			[this](QTreeWidgetItem* item) {
				if (item != m_rootItem) {
					return;
				}

				bool expandChildren = true;
				for (int i = 0; i < m_rootItem->childCount(); ++i) {
					if (m_rootItem->child(i)->isExpanded()) {
						expandChildren = false;
						break;
					}
				}

				m_rootItem->setExpanded(true);
				for (int i = 0; i < m_rootItem->childCount(); ++i) {
					m_rootItem->child(i)->setExpanded(expandChildren);
				}
			});

	/* Ctrl+C copies the cell under the cursor rather than the whole row,
	 * because what people come here to copy is one file name. */
	auto* copyShortcut = new QShortcut(QKeySequence::Copy, m_treeWidget);
	connect(copyShortcut, &QShortcut::activated, this, [this] {
		auto* current = m_treeWidget->currentItem();
		if (current == nullptr) {
			return;
		}
		const int column = m_treeWidget->currentColumn();
		const QVariant value = current->data(column, Qt::UserRole);
		QApplication::clipboard()->setText(
			value.isValid() ? value.toString() : current->text(column));
	});

	auto* bottomLayout = new QHBoxLayout();

	m_toggleDepsButton = new QPushButton(tr("Toggle Dependencies"), this);
	/* Shown by appendRow() as soon as there is a dependency row. */
	m_toggleDepsButton->setVisible(false);
	connect(m_toggleDepsButton, &QPushButton::clicked, this,
			&DownloadSummaryDialog::onToggleDependencies);
	bottomLayout->addWidget(m_toggleDepsButton);

	m_onlyCheckedLabel =
		new QLabel(tr("Only %1 with a check will be downloaded!").arg(contents),
				   this);
	m_onlyCheckedLabel->setWordWrap(true);
	bottomLayout->addWidget(m_onlyCheckedLabel, 1);

	m_cancelButton = new QPushButton(tr("Cancel"), this);
	m_continueButton = new QPushButton(tr("OK"), this);
	m_continueButton->setDefault(true);

	connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
	connect(m_continueButton, &QPushButton::clicked, this, &QDialog::accept);

	bottomLayout->addWidget(m_cancelButton);
	bottomLayout->addWidget(m_continueButton);
	layout->addLayout(bottomLayout);
}
