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

#include "CreateBackup.h"

#include "BaseInstance.h"
#include "launch/LaunchTask.h"

void CreateBackup::executeTask()
{
	auto instance = m_parent->instance();
	if (!instance) {
		emitSucceeded();
		return;
	}

	emit logLine(tr("Creating a pre-launch backup of the instance...") +
					 QStringLiteral("\n"),
				 MessageLevel::MeshMC);

	m_backupTask.reset(new BackupTask(instance->id(), instance->instanceRoot(),
									  QStringLiteral("pre-launch")));

	connect(m_backupTask.get(), &Task::finished, this,
			&CreateBackup::backupFinished);
	connect(m_backupTask.get(), &Task::progress, this, &Task::setProgress);
	connect(m_backupTask.get(), &Task::status, this, &Task::setStatus);
	connect(m_backupTask.get(), &Task::details, this, &Task::setDetails);

	// We are only a wrapper around the backup itself, so its steps have
	// to carry on through us.
	propagateStepsFrom(m_backupTask.get());
	emit progressReportingRequest();
}

void CreateBackup::proceed()
{
	m_backupTask->start();
}

void CreateBackup::backupFinished()
{
	if (m_backupTask->wasSuccessful()) {
		emit logLine(tr("Pre-launch backup created: %1")
						 .arg(m_backupTask->result().fileName) +
						 QStringLiteral("\n"),
					 MessageLevel::MeshMC);
	} else {
		// Refusing to start the game because a convenience snapshot
		// could not be written would be the worse failure of the two,
		// and it is what the plugin this replaces did as well: warn and
		// carry on.
		emit logLine(tr("Pre-launch backup failed: %1")
						 .arg(m_backupTask->failReason()) +
						 QStringLiteral("\n"),
					 MessageLevel::Warning);
	}

	// Deliberately not resetting m_backupTask here: we are inside its
	// own finished() emission. It dies with this step.
	emitSucceeded();
}
