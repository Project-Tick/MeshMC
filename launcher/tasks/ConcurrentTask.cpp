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

#include "ConcurrentTask.h"

#include <QDebug>

ConcurrentTask::ConcurrentTask(QObject* parent, QString task_name,
							   int max_concurrent)
	: Task(parent), m_max_concurrent(qMax(1, max_concurrent))
{
	if (!task_name.isEmpty()) {
		setObjectName(task_name);
	}
}

ConcurrentTask::~ConcurrentTask()
{
	// Children outlive us when someone else holds a reference to them. Make
	// sure none of them calls back into a half destroyed object.
	for (auto& task : m_doing) {
		disconnect(task.get(), nullptr, this, nullptr);
	}
}

void ConcurrentTask::addTask(Task::Ptr task)
{
	if (!task) {
		qWarning() << "ConcurrentTask" << objectName()
				   << "was handed a null task, ignoring it";
		return;
	}
	if (isFinished()) {
		qWarning() << "ConcurrentTask" << objectName()
				   << "was handed a task after it had already finished,"
				   << "ignoring it";
		return;
	}

	m_all.append(task);
	m_queue.enqueue(task);

	if (isRunning()) {
		// Through the event loop, because tasks are usually queued up in a
		// batch from executeTask(). Reacting to each one right away would let
		// us decide we are finished halfway through building the list.
		QMetaObject::invokeMethod(this, &ConcurrentTask::startNext,
								  Qt::QueuedConnection);
	}
}

void ConcurrentTask::executeTask()
{
	startNext();
}

void ConcurrentTask::startNext()
{
	if (!isRunning()) {
		// Tasks can be queued up long before anyone starts us, and children
		// can settle after we already reached an end state.
		return;
	}

	if (m_aborted) {
		dropQueued();
	}

	while (m_doing.size() < m_max_concurrent && !m_queue.isEmpty()) {
		auto task = m_queue.dequeue();

		if (task->isFinished()) {
			// Somebody else already ran this one. Take the result as it is
			// instead of starting it a second time.
			if (task->wasSuccessful()) {
				m_succeeded++;
			} else {
				m_failed++;
				m_fail_reasons.append(task->failReason());
			}
			continue;
		}

		startSubTask(task);
	}

	updateState();

	if (m_queue.isEmpty() && m_doing.isEmpty()) {
		finishTask();
	}
}

void ConcurrentTask::startSubTask(Task::Ptr task)
{
	m_doing.insert(task.get(), task);

	auto step = stepFor(task);
	step->state = TaskStepState::Running;
	step->status = task->getStatus();
	step->details = task->getDetails();
	step->current = task->getProgress();
	step->total = task->getTotalProgress();
	emit stepProgress(*step);

	connect(task.get(), &Task::succeeded, this,
			[this, task] { subTaskSucceeded(task); });
	connect(task.get(), &Task::failed, this,
			[this, task](QString reason) { subTaskFailed(task, reason); });
	connect(task.get(), &Task::status, this,
			[this, task](QString text) { subTaskStatus(task, text); });
	connect(task.get(), &Task::details, this,
			[this, task](QString text) { subTaskDetails(task, text); });
	connect(task.get(), &Task::progress, this,
			[this, task](qint64 current, qint64 total) {
				subTaskProgress(task, current, total);
			});
	// Steps of a child that is itself a multi step task belong in the same
	// list, so hand them straight to our own listeners.
	connect(task.get(), &Task::stepProgress, this,
			&ConcurrentTask::propagateStepProgress);

	if (!task->isRunning()) {
		// Start it from the event loop so that a task finishing right away
		// does not unwind back into the middle of startNext().
		QMetaObject::invokeMethod(task.get(), &Task::start,
								  Qt::QueuedConnection);
	}
}

void ConcurrentTask::disposeOf(Task::Ptr task, TaskStepState state)
{
	disconnect(task.get(), nullptr, this, nullptr);
	m_doing.remove(task.get());

	auto step = m_steps.value(task->getUid());
	if (step) {
		step->state = state;
		emit stepProgress(*step);
	}
}

void ConcurrentTask::dropQueued()
{
	m_skipped += m_queue.size();
	m_queue.clear();
}

std::shared_ptr<TaskStepProgress> ConcurrentTask::stepFor(
	const Task::Ptr& task)
{
	auto uid = task->getUid();
	auto existing = m_steps.value(uid);
	if (existing) {
		return existing;
	}

	auto step = std::make_shared<TaskStepProgress>(uid);
	m_steps.insert(uid, step);
	m_step_order.append(step);
	return step;
}

void ConcurrentTask::updateState()
{
	if (totalSize() > 1) {
		setProgress(finishedSize(), totalSize());
		setStatus(tr("Executing %1 task(s) (%2 out of %3 are done)")
					  .arg(QString::number(m_doing.size()),
						   QString::number(finishedSize()),
						   QString::number(totalSize())));
		return;
	}

	// A lone task gets no list of its own to be seen in, so we step aside and
	// let it speak through us instead. See subTaskStatus and friends.
	if (!m_queue.isEmpty()) {
		setStatus(tr("Waiting for a task to start..."));
	} else if (!m_doing.isEmpty()) {
		setStatus(tr("Executing 1 task:"));
	} else if (finishedSize() > 0) {
		setStatus(tr("Task finished."));
	} else {
		setStatus(tr("Please wait..."));
	}
}

void ConcurrentTask::finishTask()
{
	if (!isRunning()) {
		// Already succeeded, failed or aborted. Do not do it twice.
		return;
	}

	if (m_aborted) {
		emitAborted();
	} else if (m_fail_reasons.isEmpty()) {
		emitSucceeded();
	} else {
		emitFailed(m_fail_reasons.join(QStringLiteral("\n")));
	}
}

void ConcurrentTask::subTaskSucceeded(Task::Ptr task)
{
	m_succeeded++;
	disposeOf(task, TaskStepState::Succeeded);
	startNext();
}

void ConcurrentTask::subTaskFailed(Task::Ptr task, const QString& reason)
{
	m_failed++;
	if (!reason.isEmpty()) {
		m_fail_reasons.append(reason);
	}
	disposeOf(task, TaskStepState::Failed);
	startNext();
}

void ConcurrentTask::subTaskStatus(Task::Ptr task, const QString& status)
{
	auto step = m_steps.value(task->getUid());
	if (!step) {
		return;
	}
	step->status = status;
	step->state = TaskStepState::Running;
	emit stepProgress(*step);

	// With nothing but one task there is no list on screen, so its status is
	// the only one worth showing and it becomes ours.
	if (totalSize() == 1) {
		setStatus(status);
	}
}

void ConcurrentTask::subTaskDetails(Task::Ptr task, const QString& details)
{
	auto step = m_steps.value(task->getUid());
	if (!step) {
		return;
	}
	step->details = details;
	step->state = TaskStepState::Running;
	emit stepProgress(*step);

	if (totalSize() == 1) {
		setDetails(details);
	}
}

void ConcurrentTask::subTaskProgress(Task::Ptr task, qint64 current,
									 qint64 total)
{
	auto step = m_steps.value(task->getUid());
	if (!step) {
		return;
	}
	step->update(current, total);
	emit stepProgress(*step);

	// Likewise, with one task the overall bar has nothing better to track.
	if (totalSize() == 1) {
		setProgress(current, total);
	}
}

bool ConcurrentTask::canAbort() const
{
	// Anything still queued can simply be dropped, so only the tasks that are
	// already on their way have a say in this.
	for (auto& task : m_doing) {
		if (!task->canAbort()) {
			return false;
		}
	}
	return true;
}

bool ConcurrentTask::abort()
{
	m_aborted = true;
	dropQueued();

	bool fully_aborted = true;
	// Copy, because aborting a child can finish it right away and change the
	// container under us.
	const auto running = m_doing.values();
	for (auto& task : running) {
		if (task->canAbort()) {
			fully_aborted &= task->abort();
		} else {
			fully_aborted = false;
		}
	}

	if (m_doing.isEmpty()) {
		// Nothing left to wait for, so nobody else is going to wrap us up.
		finishTask();
	}

	return fully_aborted;
}
