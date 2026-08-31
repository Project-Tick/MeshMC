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

#include <QWidget>

#include "tasks/Task.h"

namespace Ui
{
	class TaskStepProgressBar;
}

/**
 * One line in the list of things a multi step task is busy with: what it is
 * doing, how far along it is, and a short detail on the right hand side.
 */
class TaskStepProgressBar : public QWidget
{
	Q_OBJECT

  public:
	explicit TaskStepProgressBar(QWidget* parent = nullptr);
	~TaskStepProgressBar();

	/// Shows the given step. A step with an unknown total gets a busy
	/// indicator rather than a made up percentage.
	void setStep(const TaskStepProgress& step);

  private:
	Ui::TaskStepProgressBar* ui;
};
