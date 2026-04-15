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

#include <QUrl>
#include <QString>

/**
 * This wraps around QDesktopServices and adds workarounds where needed
 * Use this instead of QDesktopServices!
 */
namespace DesktopServices
{
	/**
	 * Open a file in whatever application is applicable
	 */
	bool openFile(const QString& path);

	/**
	 * Open a file in the specified application
	 */
	bool openFile(const QString& application, const QString& path,
				  const QString& workingDirectory = QString(), qint64* pid = 0);

	/**
	 * Run an application
	 */
	bool run(const QString& application, const QStringList& args,
			 const QString& workingDirectory = QString(), qint64* pid = 0);

	/**
	 * Open a directory
	 */
	bool openDirectory(const QString& path, bool ensureExists = false);

	/**
	 * Open the URL, most likely in a browser. Maybe.
	 */
	bool openUrl(const QUrl& url);
} // namespace DesktopServices
