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

#include <QString>
#include <QStringList>
#include <QVector>

/*
 * PluginDependencyResolver — produces a load order for a set of discovered
 * .mmco modules that respects their declared dependencies.
 *
 * Algorithm: Kahn's topological sort over the dependency DAG built from
 * MMCOModuleInfo::dependencies. Modules that participate in a cycle are
 * marked disabled with reason DependencyCycle. Modules whose required
 * (non-optional) dependencies are missing are marked disabled with
 * reason DependencyMissing.
 *
 * The resolver does NOT mutate modules that the loader has already
 * disabled (e.g. for signature reasons) — it just refuses to include
 * them in the load order and propagates DependencyMissing to anything
 * that depended on them.
 */
class PluginDependencyResolver
{
  public:
	struct Result {
		/* Indices into the input vector, in load order. Disabled modules
		 * are NOT included here. */
		QVector<int> loadOrder;
	};

	/*
	 * Resolve dependencies for `modules` (mutated in place to set
	 * disable reasons / details for modules that cannot load).
	 *
	 * The pre-existing `disabled` flag is respected — already-disabled
	 * modules count as missing dependencies for anything that needs them.
	 */
	static Result resolve(QVector<PluginMetadata>& modules);

  private:
	/* Compare two version strings using a relaxed semver scheme. Returns
	 * negative if a < b, 0 if equal, positive if a > b. Empty/missing
	 * strings compare equal. */
	static int compareVersions(const QString& a, const QString& b);
};
