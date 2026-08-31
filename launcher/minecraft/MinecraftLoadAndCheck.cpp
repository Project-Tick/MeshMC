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
	connect(m_task.get(), &Task::details, this,
			&MinecraftLoadAndCheck::setDetails);
	// We only stand in front of the metadata load; its steps belong to us.
	propagateStepsFrom(m_task.get());
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
