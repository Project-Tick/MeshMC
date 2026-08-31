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

#include "ModFolderLoadTask.h"
#include <QDebug>

ModFolderLoadTask::ModFolderLoadTask(QDir dir)
	: m_dir(dir), m_result(new Result())
{
}

void ModFolderLoadTask::run()
{
	m_dir.refresh();
	for (auto entry : m_dir.entryInfoList()) {
		// The persistent sidecar dir lives next to the mod files and must
		// never be exposed as a "mod folder" entry.
		if (entry.isDir() && entry.fileName() == QStringLiteral(".index")) {
			continue;
		}
		Mod m(entry);
		m_result->mods[m.mmc_id()] = m;
	}
	emit succeeded();
}
