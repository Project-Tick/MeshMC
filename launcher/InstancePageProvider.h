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

#pragma once
#include "minecraft/MinecraftInstance.h"
#include "minecraft/legacy/LegacyInstance.h"
#include <FileSystem.h>
#include "ui/pages/BasePage.h"
#include "ui/pages/BasePageProvider.h"
#include "ui/pages/instance/LogPage.h"
#include "ui/pages/instance/VersionPage.h"
#include "ui/pages/instance/ModFolderPage.h"
#include "ui/pages/instance/ResourcePackPage.h"
#include "ui/pages/instance/TexturePackPage.h"
#include "ui/pages/instance/ShaderPackPage.h"
#include "ui/pages/instance/NotesPage.h"
#include "ui/pages/instance/ScreenshotsPage.h"
#include "ui/pages/instance/InstanceSettingsPage.h"
#include "ui/pages/instance/OtherLogsPage.h"
#include "ui/pages/instance/LegacyUpgradePage.h"
#include "ui/pages/instance/WorldListPage.h"
#include "ui/pages/instance/ServersPage.h"
#include "ui/pages/instance/GameOptionsPage.h"

class InstancePageProvider : public QObject, public BasePageProvider
{
	Q_OBJECT
  public:
	explicit InstancePageProvider(InstancePtr parent)
	{
		inst = parent;
	}

	virtual ~InstancePageProvider() {};
	virtual QList<BasePage*> getPages() override
	{
		QList<BasePage*> values;
		values.append(new LogPage(inst));
		std::shared_ptr<MinecraftInstance> onesix =
			std::dynamic_pointer_cast<MinecraftInstance>(inst);
		if (onesix) {
			values.append(new VersionPage(onesix.get()));
			auto modsPage = new ModFolderPage(
				onesix.get(), onesix->loaderModList(), "mods", "loadermods",
				tr("Loader mods"), "Loader-mods");
			modsPage->setFilter("%1 (*.zip *.jar *.litemod)");
			values.append(modsPage);
			values.append(new CoreModFolderPage(
				onesix.get(), onesix->coreModList(), "coremods", "coremods",
				tr("Core mods"), "Core-mods"));
			values.append(new ResourcePackPage(onesix.get()));
			values.append(new TexturePackPage(onesix.get()));
			values.append(new ShaderPackPage(onesix.get()));
			values.append(new NotesPage(onesix.get()));
			values.append(new WorldListPage(onesix.get(), onesix->worldList()));
			values.append(new ServersPage(onesix));
			// values.append(new GameOptionsPage(onesix.get()));
			values.append(new ScreenshotsPage(
				FS::PathCombine(onesix->gameRoot(), "screenshots")));
			values.append(new InstanceSettingsPage(onesix.get()));
		}
		std::shared_ptr<LegacyInstance> legacy =
			std::dynamic_pointer_cast<LegacyInstance>(inst);
		if (legacy) {
			values.append(new LegacyUpgradePage(legacy));
			values.append(new NotesPage(legacy.get()));
			values.append(new WorldListPage(legacy.get(), legacy->worldList()));
			values.append(new ScreenshotsPage(
				FS::PathCombine(legacy->gameRoot(), "screenshots")));
		}
		auto logMatcher = inst->getLogFileMatcher();
		if (logMatcher) {
			values.append(
				new OtherLogsPage(inst->getLogFileRoot(), logMatcher));
		}
		return values;
	}

	virtual QString dialogTitle() override
	{
		return tr("Edit Instance (%1)").arg(inst->name());
	}

  protected:
	InstancePtr inst;
};
