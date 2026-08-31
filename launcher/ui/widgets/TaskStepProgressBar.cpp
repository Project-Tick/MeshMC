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

#include "TaskStepProgressBar.h"
#include "ui_TaskStepProgressBar.h"

#include <limits>

TaskStepProgressBar::TaskStepProgressBar(QWidget* parent)
	: QWidget(parent), ui(new Ui::TaskStepProgressBar)
{
	ui->setupUi(this);
}

TaskStepProgressBar::~TaskStepProgressBar()
{
	delete ui;
}

void TaskStepProgressBar::setStep(const TaskStepProgress& step)
{
	ui->statusLabel->setText(step.status);
	ui->detailsLabel->setText(step.details);

	if (step.total <= 0) {
		// Nobody told us how much work there is, so sweep instead of making
		// up a percentage.
		ui->progressBar->setRange(0, 0);
		ui->progressBar->setValue(0);
		return;
	}

	// A progress bar counts in int, and byte counts do not fit in one. Work
	// in a fraction of the whole int range instead, which keeps the printed
	// percentage exact no matter how big the numbers get.
	constexpr int range = std::numeric_limits<int>::max();
	const double fraction =
		qBound(0.0,
			   static_cast<double>(step.current) /
				   static_cast<double>(step.total),
			   1.0);

	ui->progressBar->setRange(0, range);
	ui->progressBar->setValue(static_cast<int>(fraction * range));
}
