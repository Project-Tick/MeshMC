/* Copyright 2013-2021 MultiMC Contributors
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

#include <QUuid>
#include <QPointer>
#include <memory>

#include <QObject>
#include <QString>
#include <QStringList>

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

    enum class State
    {
        Inactive,
        Running,
        Succeeded,
        Failed,
        AbortedByUser
    };

public:
    explicit Task(QObject *parent = 0);
    virtual ~Task() {};

    bool isRunning() const;
    bool isFinished() const;
    bool wasSuccessful() const;
    bool wasAborted() const { return m_state == State::AbortedByUser; }

    /*!
     * Returns the string that was passed to emitFailed as the error message when the task failed.
     * If the task hasn't failed, returns an empty string.
     */
    QString failReason() const;

    virtual QStringList warnings() const;

    virtual bool isMultiStep() const { return !m_stepSources.isEmpty(); }
    virtual TaskStepProgressList getStepProgress() const;
    QString getDetails() const { return m_details; }

    virtual bool canAbort() const { return false; }

    QString getStatus()
    {
        return m_status;
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
    void propagateStepsFrom(Task* task);
    void setDetails(const QString& details);
    void logWarning(const QString & line);

private:
    QString describe();

signals:
    void started();
    void progress(qint64 current, qint64 total);
    void finished();
    void succeeded();
    void failed(QString reason);
    void status(QString status);
    void details(const QString& details);
    void stepProgress(const TaskStepProgress& step);

public slots:
    virtual void start();
    virtual bool abort() { return false; };

protected:
    virtual void executeTask() = 0;

protected slots:
    virtual void emitSucceeded();
    virtual void emitAborted();
    virtual void emitFailed(QString reason);

public slots:
    void setStatus(const QString &status);
    void setProgress(qint64 current, qint64 total);

private:
    State m_state = State::Inactive;
    QStringList m_Warnings;
    QString m_failReason = "";
    QString m_status;
    QString m_details;
    QList<QPointer<Task>> m_stepSources;
    qint64 m_progress = 0;
    qint64 m_progressTotal = 100;
};

