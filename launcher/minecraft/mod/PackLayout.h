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
