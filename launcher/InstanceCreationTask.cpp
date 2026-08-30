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

#include "InstanceCreationTask.h"
#include "settings/INISettingsObject.h"
#include "FileSystem.h"

// FIXME: remove this
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

InstanceCreationTask::InstanceCreationTask(BaseVersionPtr version)
{
	m_version = version;
}

void InstanceCreationTask::executeTask()
{
	setStatus(tr("Creating instance from version %1").arg(m_version->name()));

	/* Outlives the scope below on purpose: this function ends by handing
	 * the instance to downloadFiles(), which runs asynchronously against
	 * it. The scope is still what it was - everything that writes to
	 * instance.cfg is released before we finish - the instance is simply
	 * no longer part of what the scope owns. */
	std::shared_ptr<MinecraftInstance> instance;
	{
		auto instanceSettings = std::make_shared<INISettingsObject>(
			FS::PathCombine(m_stagingPath, "instance.cfg"));
		instanceSettings->suspendSave();
		instanceSettings->registerSetting("InstanceType", "Legacy");
		instanceSettings->set("InstanceType", "OneSix");
		instance = std::make_shared<MinecraftInstance>(
			m_globalSettings, instanceSettings, m_stagingPath);
		MinecraftInstance& inst = *instance;
		auto components = inst.getPackProfile();
		components->buildingFromScratch();
		components->setComponentVersion("net.minecraft",
										m_version->descriptor(), true);
		inst.setName(m_instName);
		inst.setIconKey(m_instIcon);
		instanceSettings->resumeSave();
	}

	/* Finishes the task, whether or not it downloads anything. */
	downloadFiles(instance);
}
