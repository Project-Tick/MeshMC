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

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QSet>

#include <memory>
#include "minecraft/OpSys.h"
#include "minecraft/Rule.h"
#include "ProblemProvider.h"
#include "Library.h"
#include <meta/JsonFormat.h>

class PackProfile;
class VersionFile;
class LaunchProfile;
struct MojangDownloadInfo;
struct MojangAssetIndexInfo;

using VersionFilePtr = std::shared_ptr<VersionFile>;
class VersionFile : public ProblemContainer
{
	friend class MojangVersionFormat;
	friend class OneSixVersionFormat;

  public: /* methods */
	void applyTo(LaunchProfile* profile);

  public: /* data */
	/// MeshMC: order hint for this version file if no explicit order is set
	int order = 0;

	/// MeshMC: human readable name of this package
	QString name;

	/// MeshMC: package ID of this package
	QString uid;

	/// MeshMC: version of this package
	QString version;

	/// MeshMC: DEPRECATED dependency on a Minecraft version
	QString dependsOnMinecraftVersion;

	/// Mojang: DEPRECATED used to version the Mojang version format
	int minimumMeshMCVersion = -1;

	/// Mojang: DEPRECATED version of Minecraft this is
	QString minecraftVersion;

	/// Mojang: class to launch Minecraft with
	QString mainClass;

	/// MeshMC: class to launch legacy Minecraft with (embed in a custom window)
	QString appletClass;

	/// Mojang: Minecraft launch arguments (may contain placeholders for
	/// variable substitution)
	QString minecraftArguments;

	/// Mojang: type of the Minecraft version
	QString type;

	/// Mojang: the time this version was actually released by Mojang
	QDateTime releaseTime;

	/// Mojang: DEPRECATED the time this version was last updated by Mojang
	QDateTime updateTime;

	/// Mojang: DEPRECATED asset group to be used with Minecraft
	QString assets;

	/// MeshMC: list of tweaker mod arguments for launchwrapper
	QStringList addTweakers;

	/// Mojang: list of libraries to add to the version
	QList<LibraryPtr> libraries;

	/// MeshMC: list of maven files to put in the libraries folder, but not in
	/// classpath
	QList<LibraryPtr> mavenFiles;

	/// The main jar (Minecraft version library, normally)
	LibraryPtr mainJar;

	/// MeshMC: list of attached traits of this version file - used to enable
	/// features
	QSet<QString> traits;

	/// MeshMC: list of jar mods added to this version
	QList<LibraryPtr> jarMods;

	/// MeshMC: list of mods added to this version
	QList<LibraryPtr> mods;

	/**
	 * MeshMC: set of packages this depends on
	 * NOTE: this is shared with the meta format!!!
	 */
	Meta::RequireSet requirements;

	/**
	 * MeshMC: set of packages this conflicts with
	 * NOTE: this is shared with the meta format!!!
	 */
	Meta::RequireSet conflicts;

	/// is volatile -- may be removed as soon as it is no longer needed by
	/// something else
	bool m_volatile = false;

  public:
	// Mojang: DEPRECATED list of 'downloads' - client jar, server jar, windows
	// server exe, maybe more.
	QMap<QString, std::shared_ptr<MojangDownloadInfo>> mojangDownloads;

	// Mojang: extended asset index download information
	std::shared_ptr<MojangAssetIndexInfo> mojangAssetIndex;
};
