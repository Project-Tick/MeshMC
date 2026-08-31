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

#include <QFileInfo>

/* Structural recognition for the pack formats that live next to mods but
 * carry no mod metadata at all. LocalModParseTask only knows how to read
 * mcmod.info / fabric.mod.json / mods.toml, so for shader packs and data
 * packs the only thing that distinguishes a usable file from a random zip
 * is its internal directory layout.
 *
 * Everything here is deliberately cheap: it reads the archive's central
 * directory (or a couple of QFileInfo stats for exploded folders) and
 * never extracts anything. It is safe to call from the GUI thread. */
namespace PackLayout
{

	/* True when `file` - a directory or a zip archive - is laid out as an
	 * OptiFine/Iris shader pack, i.e. it contains a `shaders` directory
	 * either at its root or exactly one level down. The one-level-down
	 * case matters because shader packs downloaded straight from a Git
	 * forge are wrapped in a single top-level folder. */
	bool isShaderPack(const QFileInfo& file);

	/* True when `file` is laid out as a vanilla data pack, i.e. it pairs
	 * a `pack.mcmeta` with a `data` directory. As with shader packs the
	 * pair may sit at the root or one level down. */
	bool isDataPack(const QFileInfo& file);

} // namespace PackLayout
