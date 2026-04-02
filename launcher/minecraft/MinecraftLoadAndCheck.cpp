/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "MinecraftLoadAndCheck.h"
#include "MinecraftInstance.h"
#include "PackProfile.h"

MinecraftLoadAndCheck::MinecraftLoadAndCheck(MinecraftInstance* inst,
											 QObject* parent)
	: Task(parent), m_inst(inst)
{
}

void MinecraftLoadAndCheck::executeTask()
{
	// add offline metadata load task
	auto components = m_inst->getPackProfile();
	components->reload(Net::Mode::Offline);
	m_task = components->getCurrentTask();

	if (!m_task) {
		emitSucceeded();
		return;
	}
	connect(m_task.get(), &Task::succeeded, this,
			&MinecraftLoadAndCheck::subtaskSucceeded);
	connect(m_task.get(), &Task::failed, this,
			&MinecraftLoadAndCheck::subtaskFailed);
	connect(m_task.get(), &Task::progress, this,
			&MinecraftLoadAndCheck::progress);
	connect(m_task.get(), &Task::status, this,
			&MinecraftLoadAndCheck::setStatus);
}

void MinecraftLoadAndCheck::subtaskSucceeded()
{
	if (isFinished()) {
		qCritical() << "MinecraftUpdate: Subtask" << sender()
					<< "succeeded, but work was already done!";
		return;
	}
	emitSucceeded();
}

void MinecraftLoadAndCheck::subtaskFailed(QString error)
{
	if (isFinished()) {
		qCritical() << "MinecraftUpdate: Subtask" << sender()
					<< "failed, but work was already done!";
		return;
	}
	emitFailed(error);
}
