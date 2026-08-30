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
#include "ui/pages/instance/DataPackPage.h"
#include "ui/pages/instance/NotesPage.h"
#include "ui/pages/instance/ScreenshotsPage.h"
#include "ui/pages/instance/InstanceSettingsPage.h"
#include "ui/pages/instance/OtherLogsPage.h"
#include "ui/pages/instance/LegacyUpgradePage.h"
#include "ui/pages/instance/ManagedPackPage.h"
#include "ui/pages/instance/WorldListPage.h"
#include "ui/pages/instance/ServersPage.h"
#include "ui/pages/instance/GameOptionsPage.h"
#include "ui/pages/instance/BackupPage.h"
#include "Application.h"
#include "plugin/PluginManager.h"
#include "plugin/PluginHooks.h"

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
			/* Only for instances that actually came from a modpack
			 * catalogue. Checked here rather than left to
			 * shouldDisplay() so that the page - and the network
			 * machinery behind it - is not built at all for the
			 * majority of instances, which are not managed packs. */
			if (ManagedPackPage::isSupported(onesix.get())) {
				values.append(new ManagedPackPage(onesix.get()));
			}
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
			/* Global (instance-wide) data pack folder. Hidden unless the
			 * user turns it on in instance settings - vanilla reads data
			 * packs per world, which WorldListPage handles. */
			values.append(new GlobalDataPackPage(onesix.get()));
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

		// Backups work on the instance directory as a whole, so every
		// instance type gets the page. Kept last to match where the old
		// BackupSystem plugin used to insert it (via UI_INSTANCE_PAGES).
		values.append(new BackupPage(inst.get()));

		// Let plugins add their own instance pages
		if (APPLICATION->pluginManager()) {
			QByteArray idUtf8 = inst->id().toUtf8();
			QByteArray nameUtf8 = inst->name().toUtf8();
			QByteArray pathUtf8 = inst->instanceRoot().toUtf8();
			MMCOInstancePagesEvent evt{};
			evt.instance_id = idUtf8.constData();
			evt.instance_name = nameUtf8.constData();
			evt.instance_path = pathUtf8.constData();
			evt.page_list_handle = &values;
			evt.instance_handle = inst.get();
			APPLICATION->pluginManager()->dispatchHook(
				MMCO_HOOK_UI_INSTANCE_PAGES, &evt);
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
