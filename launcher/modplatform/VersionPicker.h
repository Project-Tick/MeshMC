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

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

/* Which file of a project to install, when nobody picked one.
 *
 * The dependency resolver and the update checker both used to take
 * whatever the provider happened to list first. Neither API promises an
 * order, and CurseForge in particular will hand back a file for another
 * loader among the matches, so "first" quietly meant "a build that may
 * not even run here".
 *
 * What both of these do instead, and what the reference launcher does:
 * throw away entries built for a different loader, then take the one
 * published most recently. Deliberately not a version-string
 * comparison - mod version numbering is not semver often enough that
 * sorting by it puts "1.9" above "1.10", while the publish date is
 * stated by both providers and means the same thing on each. */
namespace ModPlatform
{

	/* One entry of a CurseForge /mods/{id}/files reply, or an empty
	 * object when nothing there is usable. `loader` may be empty, which
	 * means the caller does not know or does not care.
	 *
	 * The Minecraft version is not re-checked: the query already asked
	 * for it, and CurseForge lists a file's game versions and loaders in
	 * one and the same array, so telling a missing match from an
	 * unstated one is guesswork. */
	QJsonObject newestCurseForgeFile(const QJsonArray& files,
									 const QString& loader);

	/* The same for a Modrinth /project/{id}/version reply. */
	QJsonObject newestModrinthVersion(const QJsonArray& versions,
									  const QString& loader);

} // namespace ModPlatform
