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

#include <QVBoxLayout>
#include <QWidget>
#include <memory>

#include "ModFolderPage.h"

class ModFolderModel;

/* Data pack manager for one `datapacks` folder.
 *
 * The folder is passed in rather than derived from the instance, because
 * data packs live in two very different places: vanilla reads them from
 * saves/<world>/datapacks (one folder per world - see WorldListPage) and
 * loaders like Paxi read them from a single instance-wide folder. Both
 * behave identically once you have the model, so one page serves both. */
class DataPackPage : public ModFolderPage
{
	Q_OBJECT
  public:
	explicit DataPackPage(MinecraftInstance* instance,
						  std::shared_ptr<ModFolderModel> model,
						  QWidget* parent = nullptr);
	virtual ~DataPackPage() {}

	virtual bool shouldDisplay() const override
	{
		return true;
	}
};

/* Instance-level page for the global data pack folder.
 *
 * Wraps DataPackPage because the folder it points at can be switched off
 * entirely (GlobalDataPacksEnabled) or moved (GlobalDataPacksPath) from
 * the instance settings while this page already exists. The wrapper
 * throws the inner page away and rebuilds it whenever that happens, so
 * the page can never be left showing a folder that is no longer the
 * configured one. */
class GlobalDataPackPage : public QWidget, public BasePage
{
	Q_OBJECT
  public:
	explicit GlobalDataPackPage(MinecraftInstance* instance,
								QWidget* parent = nullptr);

	virtual QString id() const override
	{
		return "datapacks";
	}
	virtual QString displayName() const override;
	virtual QIcon icon() const override;
	virtual QString helpPage() const override;
	virtual bool shouldDisplay() const override;

	virtual bool apply() override;
	virtual void openedImpl() override;
	virtual void closedImpl() override;
	virtual void setParentContainer(BasePageContainer* container) override;

  private:
	void updateContent();

	MinecraftInstance* m_instance = nullptr;
	DataPackPage* m_underlyingPage = nullptr;
};
