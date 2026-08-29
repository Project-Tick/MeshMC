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
#include <QPointer>
#include <QString>
#include <memory>

#include "modplatform/ContentType.h"
#include "modplatform/ModDownloadTypes.h"
#include "net/NetJob.h"
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

	/* `loader` is ignored for content that has no loaders (resource
	 * packs, shader packs, data packs) - see the constructor. */
	ModUpdateCheckTask(std::shared_ptr<ModMetadataIndex> index,
					   QString mcVersion, QString loader,
					   ModPlatform::ContentType contentType,
					   QObject* parent = nullptr);

	/* Whatever was found so far. Still meaningful after abort(): the
	 * mods that were checked before the user gave up are checked, and
	 * throwing that away would only make them do it again. */
	QList<UpdateInfo> availableUpdates() const
	{
		return m_updates;
	}

	/* One lookup per tracked mod, so a large instance takes a while and
	 * the progress dialog offers an Abort button for it. */
	bool canAbort() const override
	{
		return true;
	}

  public slots:
	bool abort() override;

  protected:
	void executeTask() override;

  private:
	void onOneDone();

	std::shared_ptr<ModMetadataIndex> m_index;
	QString m_mcVersion;
	QString m_loader;
	ModPlatform::ContentType m_contentType = ModPlatform::ContentType::Mod;
	QList<UpdateInfo> m_updates;
	int m_pending = 0;
	int m_total = 0;
	int m_completed = 0;

	/* Lookups still in flight, so abort() can call them off. Guarded
	 * pointers because a job deletes itself once it has reported. */
	QList<QPointer<NetJob>> m_activeJobs;
	/* Latched by abort(): replies still arrive but are dropped, and no
	 * second verdict is given on top of the aborted one. */
	bool m_aborted = false;
};
