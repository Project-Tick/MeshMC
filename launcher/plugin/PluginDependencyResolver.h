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
