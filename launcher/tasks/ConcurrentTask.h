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

#include <QHash>
#include <QList>
#include <QQueue>
#include <QStringList>
#include <QUuid>
#include <memory>

#include "Task.h"
#include "QObjectPtr.h"

/**
 * Runs a bag of tasks, up to a given number of them at the same time.
 *
 * The point of this class over a hand rolled task list is that it reports
 * every one of its children as a separate step (see Task::stepProgress), so
 * the UI can show what each of them is doing instead of a single bar that
 * jumps around. Steps of children that are themselves multi step tasks are
 * passed along too, so nesting stays visible.
 *
 * A failing child does not stop the others here; the whole task fails once
 * everything has settled, with all the failure reasons collected. See
 * SequentialTask for the variant that stops at the first failure.
 */
class ConcurrentTask : public Task
{
	Q_OBJECT
  public:
	explicit ConcurrentTask(QObject* parent = nullptr,
							QString task_name = QString(),
							int max_concurrent = 6);
	virtual ~ConcurrentTask();

	/// Queues a task. Tasks added while this one runs are picked up as slots
	/// free up; tasks added after it finished are ignored.
	void addTask(Task::Ptr task);

	/// How many tasks were handed to us in total.
	int totalSize() const
	{
		return m_all.size();
	}

	/// How many of them reached an end state, one way or another.
	int finishedSize() const
	{
		return m_succeeded + m_failed + m_skipped;
	}

	bool isMultiStep() const override
	{
		return totalSize() > 1;
	}

	TaskStepProgressList getStepProgress() const override
	{
		return m_step_order;
	}

	bool canAbort() const override;

  public slots:
	bool abort() override;

  protected:
	void executeTask() override;

	/// Fills the free slots with queued tasks, and wraps up when there is
	/// nothing left to run.
	void startNext();

	/// Moves everything still queued out of the way without running it.
	void dropQueued();

	/// Recomputes our own progress and status line. Override to reword.
	virtual void updateState();

	/// Brings this task to an end state based on what happened to the
	/// children.
	virtual void finishTask();

	virtual void subTaskSucceeded(Task::Ptr task);
	virtual void subTaskFailed(Task::Ptr task, const QString& reason);
	virtual void subTaskStatus(Task::Ptr task, const QString& status);
	virtual void subTaskDetails(Task::Ptr task, const QString& details);
	virtual void subTaskProgress(Task::Ptr task, qint64 current, qint64 total);

	/// Hooks a task up to us and asks it to start.
	void startSubTask(Task::Ptr task);

	/// Unhooks a finished task and reports its final state to the UI.
	void disposeOf(Task::Ptr task, TaskStepState state);

	/// The step we report to the UI for the given task, created on first use.
	std::shared_ptr<TaskStepProgress> stepFor(const Task::Ptr& task);

  protected:
	/// Everything ever added, in the order it was added.
	QList<Task::Ptr> m_all;
	/// Not started yet.
	QQueue<Task::Ptr> m_queue;
	/// Started and not finished, keyed by raw pointer so lookups from signal
	/// handlers are cheap.
	QHash<Task*, Task::Ptr> m_doing;

	int m_succeeded = 0;
	int m_failed = 0;
	int m_skipped = 0;

	QStringList m_fail_reasons;

	QHash<QUuid, std::shared_ptr<TaskStepProgress>> m_steps;
	TaskStepProgressList m_step_order;

	int m_max_concurrent;
	bool m_aborted = false;
};
