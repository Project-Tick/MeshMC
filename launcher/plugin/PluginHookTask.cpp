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

#include "plugin/PluginHookTask.h"

#include <QDebug>
#include <QThreadPool>
#include <QtConcurrentRun>
#include <utility>

namespace
{
	/* The background callback running on this thread, if any.
	 *
	 * MMCOContext::progress_report only receives the module handle, not
	 * a per-call progress handle — the hook callback signature is fixed
	 * by the ABI and cannot carry one. The thread the callback runs on
	 * identifies it just as well: exactly one PluginHookTask is on the
	 * stack of any given worker thread at a time. */
	thread_local PluginHookTask* t_currentHookTask = nullptr;

	struct CurrentHookTaskGuard {
		explicit CurrentHookTaskGuard(PluginHookTask* task)
			: previous(t_currentHookTask)
		{
			t_currentHookTask = task;
		}
		~CurrentHookTaskGuard()
		{
			t_currentHookTask = previous;
		}

		PluginHookTask* previous;
	};
} // namespace

PluginHookTask::PluginHookTask(QString module_name, void* module_handle,
							   uint32_t hook_id, MMCOHookCallback callback,
							   void* payload, void* user_data, QObject* parent)
	: Task(parent), m_moduleName(std::move(module_name)),
	  m_moduleHandle(module_handle), m_hookId(hook_id), m_callback(callback),
	  m_payload(payload), m_userData(user_data)
{
	setObjectName(m_moduleName);
	setStatus(tr("Running %1...").arg(m_moduleName));
	// Nothing has told us how long this takes, and most callbacks never
	// will. Sweep until the plugin says otherwise.
	setProgress(0, 0);
}

PluginHookTask::~PluginHookTask()
{
	// The worker holds a bare pointer to us, so it must not outlive us.
	// This only ever blocks if someone tears the task down while its
	// callback is still running, which the normal flow never does.
	disconnect(&m_watcher, nullptr, this, nullptr);
	if (m_future.isRunning()) {
		qWarning() << "PluginHookTask" << m_moduleName
				   << "destroyed while its callback was still running -"
				   << "waiting for it to return";
		m_future.waitForFinished();
	}
}

PluginHookTask* PluginHookTask::currentOnThisThread()
{
	return t_currentHookTask;
}

void PluginHookTask::executeTask()
{
	if (!m_callback) {
		emitFailed(tr("%1 registered a hook without a callback.")
					   .arg(m_moduleName));
		return;
	}

	connect(&m_watcher, &QFutureWatcher<int>::finished, this,
			&PluginHookTask::callbackFinished);

	m_future = QtConcurrent::run(QThreadPool::globalInstance(), [this] {
		CurrentHookTaskGuard guard(this);
		return m_callback(m_moduleHandle, m_hookId, m_payload, m_userData);
	});
	m_watcher.setFuture(m_future);
}

void PluginHookTask::reportProgress(const QString& status,
									const QString& details, qint64 current,
									qint64 total)
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

void PluginHookTask::callbackFinished()
{
	const int rc = m_future.result();
	m_cancelRequested = rc != 0;

	if (m_cancelRequested) {
		// Non-zero is the ABI's veto, not a crash. Failing the task is
		// what stops the rest of the sequence, which is exactly what an
		// inline callback returning non-zero does to the hook chain.
		emitFailed(tr("%1 called off the operation.").arg(m_moduleName));
		return;
	}

	setProgress(1, 1);
	emitSucceeded();
}
