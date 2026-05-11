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

#pragma once

#include <QDialog>
#include <QList>
#include <QTreeWidget>

#include "modplatform/ModDownloadTypes.h"
#include "modplatform/ModUpdateCheckTask.h"

/*
 * ModUpdateDialog
 *
 * Shows the user the set of mods for which a newer compatible version was
 * found, lets them tick which updates to apply, and produces a list of
 * DownloadItem objects ready to be fed into a ContentDownloadTask.
 */
class ModUpdateDialog : public QDialog
{
	Q_OBJECT
  public:
	ModUpdateDialog(const QList<ModUpdateCheckTask::UpdateInfo>& updates,
					QWidget* parent = nullptr);

	/* Returns only the items whose row is checked. */
	QList<ModPlatform::DownloadItem> selectedDownloadItems() const;

  private:
	void setupUi(const QList<ModUpdateCheckTask::UpdateInfo>& updates);

	QTreeWidget* m_tree = nullptr;
	QList<ModUpdateCheckTask::UpdateInfo> m_updates;
};
