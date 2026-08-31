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

#include "plugin/PluginHooks.h"
#include "tasks/Task.h"

/**
 * Runs a single plugin hook callback that opted into
 * MMCO_HOOK_FLAG_BACKGROUND on a worker thread, as an ordinary launcher
 * task.
 *
 * PluginManager builds one of these per background callback of a
 * dispatch and hands them to a SequentialTask, so each plugin gets its
 * own line in the progress dialog while the GUI thread keeps running.
 *
 * The callback's return value keeps its ABI meaning: non-zero is a veto.
 * A vetoing task fails, which stops the rest of the sequence — the same
 * thing that happens when an inline callback returns non-zero.
 */
class PluginHookTask : public Task
{
	Q_OBJECT
  public:
	PluginHookTask(QString module_name, void* module_handle, uint32_t hook_id,
				   MMCOHookCallback callback, void* payload, void* user_data,
				   QObject* parent = nullptr);
	~PluginHookTask() override;

	/*!
	 * A C callback cannot be interrupted from the outside, so there is
	 * nothing meaningful to abort.
	 */
	bool canAbort() const override
	{
		return false;
	}

	/*!
	 * True when the callback returned non-zero, i.e. the plugin asked
	 * for whatever the hook was about to do to be called off. Only
	 * meaningful once the task has finished.
	 */
	bool cancelRequested() const
	{
		return m_cancelRequested;
	}

	void* moduleHandle() const
	{
		return m_moduleHandle;
	}

	/*!
	 * The task whose callback is running on the calling thread, or
	 * nullptr when the caller is not inside a background hook callback.
	 * This is how MMCOContext::progress_report finds the row to write
	 * to without the ABI having to carry a progress handle around.
	 */
	static PluginHookTask* currentOnThisThread();

	/*!
	 * Publishes a progress update coming from the plugin. Called on the
	 * worker thread; the update is marshalled onto the thread the task
	 * lives on. Empty status/details leave the current text alone.
	 */
	void reportProgress(const QString& status, const QString& details,
						qint64 current, qint64 total);

  protected:
	void executeTask() override;

  private:
	void callbackFinished();

	QString m_moduleName;
	void* m_moduleHandle;
	uint32_t m_hookId;
	MMCOHookCallback m_callback;
	void* m_payload;
	void* m_userData;

	bool m_cancelRequested = false;

	QFuture<int> m_future;
	QFutureWatcher<int> m_watcher;
};
