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
