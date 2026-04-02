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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include "MinecraftUpdate.h"
#include "MinecraftInstance.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDataStream>

#include "BaseInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/Library.h"
#include <FileSystem.h>

#include "update/FoldersTask.h"
#include "update/LibrariesTask.h"
#include "update/FMLLibrariesTask.h"
#include "update/AssetUpdateTask.h"

#include <meta/Index.h>
#include <meta/Version.h>

MinecraftUpdate::MinecraftUpdate(MinecraftInstance* inst, QObject* parent)
	: Task(parent), m_inst(inst)
{
}

void MinecraftUpdate::executeTask()
{
	m_tasks.clear();
	// create folders
	{
		m_tasks.append(std::make_shared<FoldersTask>(m_inst));
	}

	// add metadata update task if necessary
	{
		auto components = m_inst->getPackProfile();
		components->reload(Net::Mode::Online);
		auto task = components->getCurrentTask();
		if (task) {
			m_tasks.append(task.unwrap());
		}
	}

	// libraries download
	{
		m_tasks.append(std::make_shared<LibrariesTask>(m_inst));
	}

	// FML libraries download and copy into the instance
	{
		m_tasks.append(std::make_shared<FMLLibrariesTask>(m_inst));
	}

	// assets update
	{
		m_tasks.append(std::make_shared<AssetUpdateTask>(m_inst));
	}

	if (!m_preFailure.isEmpty()) {
		emitFailed(m_preFailure);
		return;
	}
	next();
}

void MinecraftUpdate::next()
{
	if (m_abort) {
		emitFailed(tr("Aborted by user."));
		return;
	}
	if (m_failed_out_of_order) {
		emitFailed(m_fail_reason);
		return;
	}
	m_currentTask++;
	if (m_currentTask > 0) {
		auto task = m_tasks[m_currentTask - 1];
		disconnect(task.get(), &Task::succeeded, this,
				   &MinecraftUpdate::subtaskSucceeded);
		disconnect(task.get(), &Task::failed, this,
				   &MinecraftUpdate::subtaskFailed);
		disconnect(task.get(), &Task::progress, this,
				   &MinecraftUpdate::progress);
		disconnect(task.get(), &Task::status, this,
				   &MinecraftUpdate::setStatus);
	}
	if (m_currentTask == m_tasks.size()) {
		emitSucceeded();
		return;
	}
	auto task = m_tasks[m_currentTask];
	// if the task is already finished by the time we look at it, skip it
	if (task->isFinished()) {
		qCritical() << "MinecraftUpdate: Skipping finished subtask"
					<< m_currentTask << ":" << task.get();
		next();
	}
	connect(task.get(), &Task::succeeded, this,
			&MinecraftUpdate::subtaskSucceeded);
	connect(task.get(), &Task::failed, this, &MinecraftUpdate::subtaskFailed);
	connect(task.get(), &Task::progress, this, &MinecraftUpdate::progress);
	connect(task.get(), &Task::status, this, &MinecraftUpdate::setStatus);
	// if the task is already running, do not start it again
	if (!task->isRunning()) {
		task->start();
	}
}

void MinecraftUpdate::subtaskSucceeded()
{
	if (isFinished()) {
		qCritical() << "MinecraftUpdate: Subtask" << sender()
					<< "succeeded, but work was already done!";
		return;
	}
	auto senderTask = QObject::sender();
	auto currentTask = m_tasks[m_currentTask].get();
	if (senderTask != currentTask) {
		qDebug() << "MinecraftUpdate: Subtask" << sender()
				 << "succeeded out of order.";
		return;
	}
	next();
}

void MinecraftUpdate::subtaskFailed(QString error)
{
	if (isFinished()) {
		qCritical() << "MinecraftUpdate: Subtask" << sender()
					<< "failed, but work was already done!";
		return;
	}
	auto senderTask = QObject::sender();
	auto currentTask = m_tasks[m_currentTask].get();
	if (senderTask != currentTask) {
		qDebug() << "MinecraftUpdate: Subtask" << sender()
				 << "failed out of order.";
		m_failed_out_of_order = true;
		m_fail_reason = error;
		return;
	}
	emitFailed(error);
}

bool MinecraftUpdate::abort()
{
	if (!m_abort) {
		m_abort = true;
		auto task = m_tasks[m_currentTask];
		if (task->canAbort()) {
			return task->abort();
		}
	}
	return true;
}

bool MinecraftUpdate::canAbort() const
{
	return true;
}
