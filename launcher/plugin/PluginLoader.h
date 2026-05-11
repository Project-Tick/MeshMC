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

#include <QSet>
#include <QStringList>
#include <QVector>

/*
 * PluginLoader — scans known directories for .mmco module files and
 * performs the low-level dlopen / symbol resolution.
 *
 * It does NOT call mmco_init(); that is the PluginManager's job.
 *
 * In addition to loading, the loader runs the *trust pre-flight*: it
 * extracts and verifies any GPG signature trailer, then applies the
 * "OSS license → signature optional / otherwise mandatory" policy. A
 * module that fails the policy is returned with `disabled=true` set so
 * that the manager skips it without unloading the shared library (the
 * library may still be needed by other tooling, e.g. the about dialog).
 *
 * Modules whose name appears in the disabled-set passed to discoverModules()
 * are likewise returned with disabled=true.
 */

class PluginLoader
{
  public:
	PluginLoader();
	~PluginLoader();

	/*
	 * Scan all configured search paths and return metadata for every
	 * valid .mmco module found.
	 *
	 * `disabledNames` is a case-insensitive set of module names that
	 * should be marked PluginDisableReason::UserDisabled. The .mmco file
	 * is still opened (so the metadata block can be displayed in the
	 * plugins dialog), but the module's `disabled` flag is set and the
	 * caller must not call mmco_init() on it.
	 */
	QVector<PluginMetadata>
	discoverModules(const QSet<QString>& disabledNames = {}) const;

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
	QVector<PluginMetadata>
	scanDirectory(const QString& dir, const QSet<QString>& disabledNames) const;

	/* Run the trust pre-flight on `meta`: verify the GPG trailer (if any)
	 * and apply the license-based signature policy. Updates the
	 * signature_state / disabled fields on `meta`. Called from
	 * loadModule() before that function returns. */
	static void verifySignatureAndPolicy(PluginMetadata& meta);
};
