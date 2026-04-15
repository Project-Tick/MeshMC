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
 *
 */

#pragma once

#include <QString>

namespace ModPlatform
{

	enum class ContentType { Mod, ResourcePack, ShaderPack };

	inline QString contentTypeToString(ContentType type)
	{
		switch (type) {
			case ContentType::Mod:
				return "mod";
			case ContentType::ResourcePack:
				return "resourcepack";
			case ContentType::ShaderPack:
				return "shader";
		}
		return "mod";
	}

	inline QString contentTypeDisplayName(ContentType type)
	{
		switch (type) {
			case ContentType::Mod:
				return "Mods";
			case ContentType::ResourcePack:
				return "Resource Packs";
			case ContentType::ShaderPack:
				return "Shader Packs";
		}
		return "Mods";
	}

	inline QString contentTypeFolderName(ContentType type)
	{
		switch (type) {
			case ContentType::Mod:
				return "mods";
			case ContentType::ResourcePack:
				return "resourcepacks";
			case ContentType::ShaderPack:
				return "shaderpacks";
		}
		return "mods";
	}

	// CurseForge classId for content type
	inline int contentTypeToCurseForgeClassId(ContentType type)
	{
		switch (type) {
			case ContentType::Mod:
				return 6; // Mods
			case ContentType::ResourcePack:
				return 12; // Resource Packs
			case ContentType::ShaderPack:
				return 6552; // Shaders
		}
		return 6;
	}

	// Modrinth project_type facet
	inline QString contentTypeToModrinthFacet(ContentType type)
	{
		switch (type) {
			case ContentType::Mod:
				return "mod";
			case ContentType::ResourcePack:
				return "resourcepack";
			case ContentType::ShaderPack:
				return "shader";
		}
		return "mod";
	}

	// CurseForge modLoaderType parameter
	// 1=Forge, 4=Fabric, 5=Quilt, 6=NeoForge
	inline int loaderToCurseForgeModLoaderType(const QString& loader)
	{
		if (loader == "forge")
			return 1;
		if (loader == "fabric")
			return 4;
		if (loader == "quilt")
			return 5;
		if (loader == "neoforge")
			return 6;
		return 0; // unknown / any
	}

} // namespace ModPlatform
