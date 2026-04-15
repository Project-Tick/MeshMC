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

#include <QJsonArray>
#include <QJsonDocument>

#include <QDebug>

#include "minecraft/VersionFile.h"
#include "minecraft/Library.h"
#include "minecraft/PackProfile.h"
#include "ParseUtils.h"

#include <Version.h>

static bool isMinecraftVersion(const QString& uid)
{
	return uid == "net.minecraft";
}

void VersionFile::applyTo(LaunchProfile* profile)
{
	// Only real Minecraft can set those. Don't let anything override them.
	if (isMinecraftVersion(uid)) {
		profile->applyMinecraftVersion(minecraftVersion);
		profile->applyMinecraftVersionType(type);
		// HACK: ignore assets from other version files than Minecraft
		// workaround for stupid assets issue caused by amazon:
		// https://www.theregister.co.uk/2017/02/28/aws_is_awol_as_s3_goes_haywire/
		profile->applyMinecraftAssets(mojangAssetIndex);
	}

	profile->applyMainJar(mainJar);
	profile->applyMainClass(mainClass);
	profile->applyAppletClass(appletClass);
	profile->applyMinecraftArguments(minecraftArguments);
	profile->applyTweakers(addTweakers);
	profile->applyJarMods(jarMods);
	profile->applyMods(mods);
	profile->applyTraits(traits);

	for (auto library : libraries) {
		profile->applyLibrary(library);
	}
	for (auto mavenFile : mavenFiles) {
		profile->applyMavenFile(mavenFile);
	}
	profile->applyProblemSeverity(getProblemSeverity());
}

/*
	auto theirVersion = profile->getMinecraftVersion();
	if (!theirVersion.isNull() && !dependsOnMinecraftVersion.isNull())
	{
		if (QRegExp(dependsOnMinecraftVersion, Qt::CaseInsensitive,
   QRegExp::Wildcard).indexIn(theirVersion) == -1)
		{
			throw MinecraftVersionMismatch(uid, dependsOnMinecraftVersion,
   theirVersion);
		}
	}
*/
