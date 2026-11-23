/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
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
