/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
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
