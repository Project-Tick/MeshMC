/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include <QDialog>
#include <QHash>
#include <QUuid>
#include <memory>

#include "tasks/Task.h"

class TaskStepProgressBar;

namespace Ui
{
	class ProgressDialog;
}

class ProgressDialog : public QDialog
{
	Q_OBJECT

  public:
	explicit ProgressDialog(QWidget* parent = 0);
	~ProgressDialog();

	/// Fits the dialog around whatever it is showing right now. Pass true to
	/// centre it on its parent instead of around its own old position.
	void updateSize(bool recenterParent = false);

	int execWithTask(Task* task);
	int execWithTask(std::unique_ptr<Task>&& task);
	int execWithTask(std::unique_ptr<Task>& task);

	void setSkipButton(bool present, QString label = QString());

	Task* getTask();

  public slots:
	void onTaskStarted();
	void onTaskFailed(QString failure);
	void onTaskSucceeded();

	void changeStatus(const QString& status);
	void changeDetails(const QString& details);
	void changeProgress(qint64 current, qint64 total);
	/// Adds, updates or retires the line belonging to one step of a multi
	/// step task.
	void changeStepProgress(const TaskStepProgress& step);

  private slots:
	void on_skipButton_clicked(bool checked);

  protected:
	virtual void keyPressEvent(QKeyEvent* e);
	virtual void closeEvent(QCloseEvent* e);

  private:
	bool handleImmediateResult(QDialog::DialogCode& result);

  private:
	Ui::ProgressDialog* ui;

	Task* task;

	/// One line per step that is currently in flight, keyed by the step's uid.
	/// Lines are dropped as their step reaches an end state.
	QHash<QUuid, TaskStepProgressBar*> m_step_bars;

	/// Everything we hooked up to the task, so it can be undone on the way
	/// out even if the task lives on.
	QList<QMetaObject::Connection> m_task_connections;
};
