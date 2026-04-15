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

#pragma once

#include <QDialog>
#include <QList>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>

#include "modplatform/ModDownloadTypes.h"

class DownloadSummaryDialog : public QDialog
{
	Q_OBJECT

  public:
	explicit DownloadSummaryDialog(
		const QList<ModPlatform::SelectedMod>& selectedMods,
		const QList<ModPlatform::DependencyInfo>& dependencies,
		const QList<ModPlatform::UnresolvedDep>& unresolvedDeps,
		QWidget* parent = nullptr);

	QList<ModPlatform::DownloadItem> downloadItems() const
	{
		return m_downloadItems;
	}

  private:
	void setupUi();

  private:
	QList<ModPlatform::SelectedMod> m_selectedMods;
	QList<ModPlatform::DependencyInfo> m_dependencies;
	QList<ModPlatform::UnresolvedDep> m_unresolvedDeps;
	QList<ModPlatform::DownloadItem> m_downloadItems;

	QTreeWidget* m_treeWidget = nullptr;
	QLabel* m_summaryLabel = nullptr;
	QPushButton* m_continueButton = nullptr;
	QPushButton* m_cancelButton = nullptr;
};
