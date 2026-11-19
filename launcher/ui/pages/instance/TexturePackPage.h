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

#pragma once

#include "ModFolderPage.h"
#include "ui_ModFolderPage.h"

class TexturePackPage : public ModFolderPage
{
	Q_OBJECT
  public:
	explicit TexturePackPage(MinecraftInstance* instance, QWidget* parent = 0)
		: ModFolderPage(instance, instance->texturePackList(), "texturepacks",
						"resourcepacks", tr("Texture packs"), "Texture-packs",
						parent)
	{
		ui->actionView_configs->setVisible(false);
		/* Legacy texture packs and modern resource packs are the same
		 * product on CurseForge and Modrinth - only the destination folder
		 * differs, and that comes from texturePackList(). Without this the
		 * page inherited ModFolderPage's ContentType::Mod default and the
		 * download button searched for (and installed) mods. */
		setContentType(ModPlatform::ContentType::ResourcePack);
	}
	virtual ~TexturePackPage() {}

	virtual bool shouldDisplay() const override
	{
		return m_inst->traits().contains("texturepacks");
	}
};
