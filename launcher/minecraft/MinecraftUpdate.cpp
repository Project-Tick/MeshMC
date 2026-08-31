/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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
