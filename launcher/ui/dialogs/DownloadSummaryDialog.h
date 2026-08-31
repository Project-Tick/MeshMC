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

#include <QDialog>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QString>
#include <QTreeWidget>

#include "modplatform/ContentType.h"
#include "modplatform/ModDownloadTypes.h"

/* The last stop before anything is downloaded: what was picked, what
 * had to come with it, and what could not be found.
 *
 * Every row carries a tick box, because the list is not only what the
 * user chose - dependency resolution adds to it, and something already
 * on disk at another version is offered rather than forced. Only ticked
 * rows are downloaded, which is what the label under the list says.
 *
 * Laid out like the reference launcher's review box: one root item
 * holding a row per file, each row expandable into its file name,
 * provider, what required it and what kind of build it is. The root is
 * tri-state, so it works as tick-all. */
class DownloadSummaryDialog : public QDialog
{
	Q_OBJECT

  public:
	explicit DownloadSummaryDialog(
		const QList<ModPlatform::SelectedMod>& selectedMods,
		const QList<ModPlatform::DependencyInfo>& dependencies,
		const QList<ModPlatform::UnresolvedDep>& unresolvedDeps,
		ModPlatform::ContentType contentType, QWidget* parent = nullptr);

	/* Only the rows that are still ticked. */
	QList<ModPlatform::DownloadItem> downloadItems() const;

  private slots:
	/* Ticks or unticks every dependency row at once - useful when the
	 * resolver brought in a dozen libraries and none of them are
	 * wanted. */
	void onToggleDependencies();

  private:
	void setupUi();
	/* One row, plus its detail lines. `enabled` decides whether it
	 * starts ticked. */
	void appendRow(const ModPlatform::DownloadItem& item,
				   const QString& provider, const QStringList& requiredBy,
				   const QString& versionType, bool isDependency,
				   bool enabled);

  private:
	QList<ModPlatform::SelectedMod> m_selectedMods;
	QList<ModPlatform::DependencyInfo> m_dependencies;
	QList<ModPlatform::UnresolvedDep> m_unresolvedDeps;
	ModPlatform::ContentType m_contentType;

	/* A row and the download it stands for, so the answer can be built
	 * from the tick boxes without matching rows back up by name. */
	struct Row {
		QTreeWidgetItem* item = nullptr;
		ModPlatform::DownloadItem download;
	};
	QList<Row> m_rows;

	QTreeWidget* m_treeWidget = nullptr;
	QTreeWidgetItem* m_rootItem = nullptr;
	QLabel* m_explainLabel = nullptr;
	QLabel* m_onlyCheckedLabel = nullptr;
	QPushButton* m_toggleDepsButton = nullptr;
	QPushButton* m_continueButton = nullptr;
	QPushButton* m_cancelButton = nullptr;

	/* Rows the resolver added, for the toggle button. */
	QList<QTreeWidgetItem*> m_dependencyItems;
	bool m_dependenciesChecked = true;
};
