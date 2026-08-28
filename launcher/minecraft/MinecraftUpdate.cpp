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
	: SequentialTask(parent, QStringLiteral("MinecraftUpdate")), m_inst(inst)
{
}

void MinecraftUpdate::executeTask()
{
	// create folders
	{
		addTask(Task::Ptr(new FoldersTask(m_inst)));
	}

	// add metadata update task if necessary
	{
		auto components = m_inst->getPackProfile();
		components->reload(Net::Mode::Online);
		auto task = components->getCurrentTask();
		if (task) {
			addTask(task);
		}
	}

	// libraries download
	{
		addTask(Task::Ptr(new LibrariesTask(m_inst)));
	}

	// FML libraries download and copy into the instance
	{
		addTask(Task::Ptr(new FMLLibrariesTask(m_inst)));
	}

	// assets update
	{
		addTask(Task::Ptr(new AssetUpdateTask(m_inst)));
	}

	SequentialTask::executeTask();
}

bool MinecraftUpdate::abort()
{
	SequentialTask::abort();
	// A step that cannot be interrupted still has to run to completion, but
	// the update as a whole stops either way, so from the caller's point of
	// view this always works.
	return true;
}

bool MinecraftUpdate::canAbort() const
{
	return true;
}
