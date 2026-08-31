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

#include <QString>

namespace ModPlatform
{

	/* NOTE: there is deliberately no separate TexturePack entry. Legacy
	 * "texture packs" and modern "resource packs" are the same product on
	 * both CurseForge and Modrinth; only the on-disk folder differs, and
	 * that is decided by the folder model, not by this enum. The legacy
	 * texture pack page therefore runs as ContentType::ResourcePack. */
	enum class ContentType { Mod, ResourcePack, ShaderPack, DataPack };

	inline QString contentTypeToString(ContentType type)
	{
		switch (type) {
			case ContentType::Mod:
				return "mod";
			case ContentType::ResourcePack:
				return "resourcepack";
			case ContentType::ShaderPack:
				return "shader";
			case ContentType::DataPack:
				return "datapack";
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
			case ContentType::DataPack:
				return "Data Packs";
		}
		return "Mods";
	}

	/* Lower-case singular noun, for use inside a sentence - "Select mod
	 * for download", "No versions for this resource pack". */
	inline QString contentTypeNoun(ContentType type)
	{
		switch (type) {
			case ContentType::Mod:
				return "mod";
			case ContentType::ResourcePack:
				return "resource pack";
			case ContentType::ShaderPack:
				return "shader pack";
			case ContentType::DataPack:
				return "data pack";
		}
		return "mod";
	}

	/* Plural of contentTypeNoun(). */
	inline QString contentTypeNounPlural(ContentType type)
	{
		return contentTypeNoun(type) + QStringLiteral("s");
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
			case ContentType::DataPack:
				return "datapacks";
		}
		return "mods";
	}

	/* Whether this kind of content is loader-specific. Resource packs,
	 * shader packs and data packs are not, so the loader filter must not
	 * be sent to either platform for them - doing so returns an empty
	 * result set on Modrinth. */
	inline bool contentTypeUsesLoader(ContentType type)
	{
		return type == ContentType::Mod;
	}

	/* Whether the download dialog offers a filter panel for this kind of
	 * content. Only mods do, matching the reference launcher: for a
	 * resource pack there is no loader to pick and no environment to
	 * narrow, which leaves too little to justify the panel. Separate
	 * from contentTypeUsesLoader() despite agreeing today, because the
	 * two answer different questions. */
	inline bool contentTypeSupportsFiltering(ContentType type)
	{
		return type == ContentType::Mod;
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
			case ContentType::DataPack:
				return 6945; // Data Packs
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
			case ContentType::DataPack:
				return "datapack";
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
