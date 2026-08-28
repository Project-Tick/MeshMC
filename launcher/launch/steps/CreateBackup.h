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

#include "backup/BackupTask.h"
#include "launch/LaunchStep.h"

/*
 * CreateBackup — takes a "pre-launch" snapshot of the instance before
 * the game starts.
 *
 * Only prepended by LaunchController when the BackupBeforeLaunch setting
 * is on, so the step itself never has to second-guess the setting.
 *
 * A step rather than an inline call in LaunchController: the archive is
 * written on a worker thread by BackupTask, and being a step is what
 * gets the progress in front of the user (instance window / progress
 * dialog) while the launch waits for it. That is the core equivalent of
 * the MMCO_HOOK_FLAG_BACKGROUND registration the BackupSystem plugin
 * used for the same job.
 */
class CreateBackup : public LaunchStep
{
	Q_OBJECT
  public:
	explicit CreateBackup(LaunchTask* parent) : LaunchStep(parent) {}
	~CreateBackup() override = default;

	void executeTask() override;
	void proceed() override;

	bool canAbort() const override
	{
		return false;
	}

  private slots:
	void backupFinished();

  private:
	shared_qobject_ptr<BackupTask> m_backupTask;
};
