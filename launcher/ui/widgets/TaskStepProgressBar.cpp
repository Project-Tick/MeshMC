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
