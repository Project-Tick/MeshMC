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

#include "CreateGameFolders.h"
#include "minecraft/MinecraftInstance.h"
#include "launch/LaunchTask.h"
#include "FileSystem.h"

CreateGameFolders::CreateGameFolders(LaunchTask* parent) : LaunchStep(parent) {}

void CreateGameFolders::executeTask()
{
	auto instance = m_parent->instance();
	std::shared_ptr<MinecraftInstance> minecraftInstance =
		std::dynamic_pointer_cast<MinecraftInstance>(instance);

	if (!FS::ensureFolderPathExists(minecraftInstance->gameRoot())) {
		emit logLine("Couldn't create the main game folder",
					 MessageLevel::Error);
		emitFailed(tr("Couldn't create the main game folder"));
		return;
	}

	// HACK: this is a workaround for MCL-3732 - 'server-resource-packs' folder
	// is created.
	if (!FS::ensureFolderPathExists(FS::PathCombine(
			minecraftInstance->gameRoot(), "server-resource-packs"))) {
		emit logLine("Couldn't create the 'server-resource-packs' folder",
					 MessageLevel::Error);
	}
	emitSucceeded();
}
