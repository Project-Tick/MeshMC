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

#include "ModFolderPage.h"
#include "ui_ModFolderPage.h"

class ShaderPackPage : public ModFolderPage
{
	Q_OBJECT
  public:
	explicit ShaderPackPage(MinecraftInstance* instance, QWidget* parent = 0)
		: ModFolderPage(instance, instance->shaderPackList(), "shaderpacks",
						"shaderpacks", tr("Shader packs"), "Resource-packs",
						parent)
	{
		ui->actionView_configs->setVisible(false);
	}
	virtual ~ShaderPackPage() {}

	virtual bool shouldDisplay() const override
	{
		return true;
	}
};
