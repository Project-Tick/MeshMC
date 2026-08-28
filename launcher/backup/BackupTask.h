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

#include <QFuture>
#include <QFutureWatcher>
#include <QString>

#include "backup/BackupManager.h"
#include "tasks/Task.h"

/*
 * BackupTask — creates one instance backup on a worker thread.
 *
 * Compressing a whole instance takes long enough that doing it on the
 * GUI thread freezes the window for the duration; that freeze is the
 * reason the BackupSystem plugin was rewritten to use the host's
 * background-hook facility before it moved into core. Here the same job
 * is an ordinary Task, so the launch sequence and the backup page get
 * off-thread execution, a real progress bar and the usual failure
 * reporting for free.
 *
 * The threading is the same shape as PluginHookTask / InstanceCopyTask:
 * QtConcurrent::run() on the global pool plus a QFutureWatcher, with
 * progress marshalled back onto the task's own thread.
 */
class BackupTask : public Task
{
	Q_OBJECT
  public:
	/*!
	 * \param label Optional label folded into the backup file name,
	 *        e.g. "pre-launch".
	 */
	BackupTask(QString instanceId, QString instanceRoot, QString label = {},
			   QObject* parent = nullptr);
	~BackupTask() override;

	/*!
	 * A partially written archive is of no use to anybody and
	 * MMCZip::compressDir() has no interruption point, so there is
	 * nothing meaningful to abort.
	 */
	bool canAbort() const override
	{
		return false;
	}

	/*!
	 * The backup that was created. Only meaningful once the task has
	 * succeeded.
	 */
	const BackupEntry& result() const
	{
		return m_result;
	}

  protected:
	void executeTask() override;

  private:
	/* Called on the worker thread; hops onto our own thread before
	 * touching anything the UI is connected to. */
	void reportProgress(const QString& status, const QString& details,
						qint64 current, qint64 total);
	void backupFinished();

	QString m_instanceId;
	QString m_instanceRoot;
	QString m_label;
	BackupEntry m_result;

	QFuture<BackupEntry> m_future;
	QFutureWatcher<BackupEntry> m_watcher;
};
