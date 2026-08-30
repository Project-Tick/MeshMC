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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QUuid>
#include <memory>

#include "QObjectPtr.h"

/**
 * The lifecycle of a single step of a multi step task, as seen by the UI.
 */
enum class TaskStepState { Waiting, Running, Failed, Succeeded };

Q_DECLARE_METATYPE(TaskStepState)

/**
 * A snapshot of one step of a multi step task.
 *
 * Multi step tasks (see Task::isMultiStep) report every step they are made of
 * through Task::stepProgress, so the UI can show one line per step instead of
 * a single opaque progress bar. A step keeps the same uid for its whole
 * lifetime, which lets the UI update the line it already has instead of piling
 * up new ones.
 */
struct TaskStepProgress {
	QUuid uid;
	qint64 current = 0;
	qint64 total = -1;

	QString status;
	QString details;
	TaskStepState state = TaskStepState::Waiting;

	TaskStepProgress() : uid(QUuid::createUuid()) {}
	explicit TaskStepProgress(QUuid step_uid) : uid(step_uid) {}

	bool isDone() const
	{
		return state == TaskStepState::Failed ||
			   state == TaskStepState::Succeeded;
	}

	void update(qint64 new_current, qint64 new_total)
	{
		current = new_current;
		total = new_total;
		state = TaskStepState::Running;
	}
};

Q_DECLARE_METATYPE(TaskStepProgress)

using TaskStepProgressList = QList<std::shared_ptr<TaskStepProgress>>;

class Task : public QObject
{
	Q_OBJECT
  public:
	using Ptr = shared_qobject_ptr<Task>;

	enum class State { Inactive, Running, Succeeded, Failed, AbortedByUser };

  public:
	explicit Task(QObject* parent = 0);
	virtual ~Task() {};

	bool isRunning() const;
	bool isFinished() const;
	bool wasSuccessful() const;

	/*!
	 * A multi step task is a task that is made up of several other tasks.
	 * Such a task reports the state of each of its steps through the
	 * stepProgress signal, on top of its own overall progress.
	 */
	virtual bool isMultiStep() const
	{
		return false;
	}

	/*!
	 * The state of every step this task is currently made of, for callers that
	 * connect to an already running task and need to catch up.
	 */
	virtual TaskStepProgressList getStepProgress() const
	{
		return {};
	}

	/*!
	 * Identifies this task among the steps of its parent. Stable for the whole
	 * lifetime of the task.
	 */
	QUuid getUid() const
	{
		return m_uid;
	}

	/*!
	 * Returns the string that was passed to emitFailed as the error message
	 * when the task failed. If the task hasn't failed, returns an empty string.
	 */
	QString failReason() const;

	virtual QStringList warnings() const;

	virtual bool canAbort() const
	{
		return false;
	}

	QString getStatus()
	{
		return m_status;
	}

	/*!
	 * Secondary, usually shorter piece of status shown next to the status
	 * line - a byte count, an item count, that kind of thing.
	 */
	QString getDetails()
	{
		return m_details;
	}

	qint64 getProgress()
	{
		return m_progress;
	}

	qint64 getTotalProgress()
	{
		return m_progressTotal;
	}

  protected:
	void logWarning(const QString& line);

	/*!
	 * Announce whatever canAbort() currently answers.
	 *
	 * Called by a task at the points where that answer changes. Reading it
	 * back through the virtual rather than taking a parameter means the
	 * button and abort() itself can never disagree about whether there is
	 * anything to abort.
	 */
	void reportAbortStatus()
	{
		emit abortStatusChanged(canAbort());
	}

	/*! Relabel the button offering abort() - see abortButtonTextChanged. */
	void setAbortButtonText(const QString& text)
	{
		emit abortButtonTextChanged(text);
	}

  private:
	QString describe();

  signals:
	void started();
	void progress(qint64 current, qint64 total);
	void finished();
	void succeeded();
	void failed(QString reason);
	void status(QString status);
	void details(QString details);
	//! Emitted by multi step tasks whenever one of their steps moves along.
	void stepProgress(TaskStepProgress const& step_progress);

	/*!
	 * The answer to canAbort() has changed.
	 *
	 * canAbort() is computed rather than stored, so nothing can tell from
	 * the outside when it starts or stops being true - a task that changes
	 * its mind mid-run has to say so. A task that never emits this is a
	 * task whose abort button keeps whatever state the caller gave it,
	 * which is what every task did before this existed.
	 */
	void abortStatusChanged(bool abortable);

	/*!
	 * The button offering abort() should be relabelled.
	 *
	 * Because "abort" is not always what pressing it does. A task that has
	 * already produced its result and is only running an optional extra
	 * step is not cancelled by that button - the step is skipped and the
	 * task still succeeds. Saying "Abort" there promises a rollback that
	 * is not going to happen.
	 */
	void abortButtonTextChanged(QString text);

  public slots:
	virtual void start();
	virtual bool abort()
	{
		return false;
	};

  protected:
	virtual void executeTask() = 0;

  protected slots:
	virtual void emitSucceeded();
	virtual void emitAborted();
	virtual void emitFailed(QString reason);

	/*!
	 * Passes a step reported by a child task on to our own listeners, so that
	 * steps of nested multi step tasks stay visible all the way up.
	 */
	virtual void propagateStepProgress(TaskStepProgress const& step_progress);

	/*!
	 * Wires an inner task so that its steps show up among ours. Use this when
	 * a task hands the actual work to a single task of its own, which would
	 * otherwise report its steps to nobody.
	 */
	void propagateStepsFrom(Task* other);

  public slots:
	void setStatus(const QString& status);
	void setDetails(const QString& details);
	void setProgress(qint64 current, qint64 total);

  private:
	State m_state = State::Inactive;
	QStringList m_Warnings;
	QString m_failReason = "";
	QString m_status;
	QString m_details;
	int m_progress = 0;
	int m_progressTotal = 100;
	QUuid m_uid = QUuid::createUuid();
};
