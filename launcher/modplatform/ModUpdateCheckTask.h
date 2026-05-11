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

#include <QList>
#include <QObject>
#include <QString>
#include <memory>

#include "modplatform/ModDownloadTypes.h"
#include "tasks/Task.h"

class ModMetadataIndex;

/*
 * ModUpdateCheckTask
 *
 * Walks every entry of a ModMetadataIndex that has a remote provenance and
 * asks the source platform for the newest compatible version. Whenever the
 * remote version differs from the recorded one, a ready-to-download
 * DownloadItem is produced (with replaceExisting + replacesFileName already
 * populated).
 *
 * This task is read-only with respect to the index — it never mutates the
 * sidecars itself. The caller is expected to hand the resulting plan to a
 * ContentDownloadTask, which both writes the new files and updates the
 * sidecars on success.
 */
class ModUpdateCheckTask : public Task
{
	Q_OBJECT
  public:
	struct UpdateInfo {
		QString currentFileName;
		QString currentVersionId;
		QString newVersionId;
		QString name;
		QString platform;
		ModPlatform::DownloadItem item;
	};

	ModUpdateCheckTask(std::shared_ptr<ModMetadataIndex> index,
					   QString mcVersion, QString loader,
					   QObject* parent = nullptr);

	QList<UpdateInfo> availableUpdates() const
	{
		return m_updates;
	}

  protected:
	void executeTask() override;

  private:
	void onOneDone();

	std::shared_ptr<ModMetadataIndex> m_index;
	QString m_mcVersion;
	QString m_loader;
	QList<UpdateInfo> m_updates;
	int m_pending = 0;
	int m_total = 0;
	int m_completed = 0;
};
