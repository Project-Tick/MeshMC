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

#include "plugin/PluginMetadata.h"

#include <QStringList>
#include <QVector>

/*
 * PluginLoader — scans known directories for .mmco module files and
 * performs the low-level dlopen / symbol resolution.
 *
 * It does NOT call mmco_init(); that is the PluginManager's job.
 */

class PluginLoader
{
  public:
	PluginLoader();
	~PluginLoader();

	/*
	 * Scan all configured search paths and return metadata for every
	 * valid .mmco module found. Invalid modules (bad magic, ABI mismatch,
	 * missing symbols) are logged and skipped.
	 */
	QVector<PluginMetadata> discoverModules() const;

	/*
	 * Open a single .mmco file: dlopen, validate magic/ABI, resolve
	 * entry points. On success the returned PluginMetadata has
	 * loaded == true. On failure loaded == false and libraryHandle
	 * is nullptr.
	 */
	PluginMetadata loadModule(const QString& path) const;

	/*
	 * Close a previously loaded module.
	 */
	static void unloadModule(PluginMetadata& meta);

	/*
	 * Return the ordered list of directories that will be scanned.
	 */
	QStringList searchPaths() const;

	/*
	 * Prepend extra search paths (e.g. from settings).
	 */
	void addSearchPath(const QString& path);

  private:
	QStringList m_extraPaths;

	static QStringList defaultSearchPaths();
	QVector<PluginMetadata> scanDirectory(const QString& dir) const;
};
