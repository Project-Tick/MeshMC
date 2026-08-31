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

#include "FoldersTask.h"
#include "minecraft/MinecraftInstance.h"
#include <QDir>

FoldersTask::FoldersTask(MinecraftInstance* inst) : Task()
{
	m_inst = inst;
}

void FoldersTask::executeTask()
{
	// Make directories
	QDir mcDir(m_inst->gameRoot());
	if (!mcDir.exists() && !mcDir.mkpath(".")) {
		emitFailed(tr("Failed to create folder for minecraft binaries."));
		return;
	}
	emitSucceeded();
}
