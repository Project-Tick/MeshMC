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

#include "backup/BackupTask.h"

#include <QDebug>
#include <QThreadPool>
#include <QtConcurrentRun>
#include <utility>

BackupTask::BackupTask(QString instanceId, QString instanceRoot, QString label,
					   QObject* parent)
	: Task(parent), m_instanceId(std::move(instanceId)),
	  m_instanceRoot(std::move(instanceRoot)), m_label(std::move(label))
{
	setObjectName(QStringLiteral("BackupTask"));
	setStatus(tr("Preparing backup..."));
	// The file count is only known once compressDir() has walked the
	// instance, so sweep until BackupManager says otherwise.
	setProgress(0, 0);
}

BackupTask::~BackupTask()
{
	// The worker holds a bare pointer to us, so it must not outlive us.
	// This only ever blocks if someone tears the task down while the
	// backup is still running, which the normal flow never does.
	disconnect(&m_watcher, nullptr, this, nullptr);
	if (m_future.isRunning()) {
		qWarning() << "BackupTask destroyed while the backup was still "
					  "running - waiting for it to finish";
		m_future.waitForFinished();
	}
}

void BackupTask::executeTask()
{
	if (m_instanceRoot.isEmpty()) {
		emitFailed(tr("The instance has no directory to back up."));
		return;
	}

	connect(&m_watcher, &QFutureWatcher<BackupEntry>::finished, this,
			&BackupTask::backupFinished);

	m_future = QtConcurrent::run(QThreadPool::globalInstance(), [this] {
		BackupManager manager(m_instanceId, m_instanceRoot);
		return manager.createBackup(
			m_label, [this](const QString& status, const QString& details,
							qint64 current, qint64 total) {
				reportProgress(status, details, current, total);
			});
	});
	m_watcher.setFuture(m_future);
}

void BackupTask::reportProgress(const QString& status, const QString& details,
								qint64 current, qint64 total)
{
	// Called from the worker thread: hop back to the thread we live on
	// before touching anything the UI is connected to.
	QMetaObject::invokeMethod(
		this,
		[this, status, details, current, total] {
			if (!status.isEmpty()) {
				setStatus(status);
			}
			if (!details.isEmpty()) {
				setDetails(details);
			}
			setProgress(current, total);
		},
		Qt::QueuedConnection);
}

void BackupTask::backupFinished()
{
	m_result = m_future.result();

	if (!m_result.isValid()) {
		emitFailed(
			tr("Could not create the backup. See the launcher log for "
			   "details."));
		return;
	}

	setProgress(1, 1);
	emitSucceeded();
}
