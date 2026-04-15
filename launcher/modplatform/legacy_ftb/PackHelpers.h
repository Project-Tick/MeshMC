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

#include <QList>
#include <QString>
#include <QStringList>
#include <QMetaType>

namespace LegacyFTB
{

	// Header for structs etc...
	enum class PackType { Public, ThirdParty, Private };

	struct Modpack {
		QString name;
		QString description;
		QString author;
		QStringList oldVersions;
		QString currentVersion;
		QString mcVersion;
		QString mods;
		QString logo;

		// Technical data
		QString dir;
		QString file; //<- Url in the xml, but doesn't make much sense

		bool bugged = false;
		bool broken = false;

		PackType type;
		QString packCode;
	};

	typedef QList<Modpack> ModpackList;

} // namespace LegacyFTB

// We need it for the proxy model
Q_DECLARE_METATYPE(LegacyFTB::Modpack)
