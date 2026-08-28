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

#include <QLatin1String>
#include <QString>

/*
 * ─── Plugins that graduated into core ─────────────────────────────────
 *
 * Every module listed here once shipped as an in-tree .mmco plugin and
 * has since been absorbed into the launcher binary. Their old .mmco
 * files may still be sitting in a user's mmcmodules/ directory (or in a
 * distro package that has not caught up yet), and loading one now would
 * double up on everything core already does: two "Backups" tabs in the
 * instance dialog, two pre-launch snapshots per launch, two news
 * dialogs fighting over the same toolbar button.
 *
 * So the loader refuses to initialise them and reports the reason in
 * the plugins dialog instead of silently ignoring the file.
 *
 * Matching is case-insensitive and checks both the module's declared
 * name (MMCO_DEFINE_MODULE) and its file name, because a renamed .mmco
 * would otherwise slip past.
 *
 * ── Adding an entry ──
 * When a plugin graduates, append it here in the same commit that adds
 * its core implementation. Keep the detail string short: it is shown
 * verbatim in the plugins dialog next to the module name.
 */

struct CoreSupersededPlugin {
	QLatin1String moduleName;
	QLatin1String detail;
};

/*
 * Returns the matching entry, or nullptr when `name` is not a module
 * that core has taken over.
 */
inline const CoreSupersededPlugin*
findCoreSupersededPlugin(const QString& name)
{
	static const CoreSupersededPlugin entries[] = {
		{QLatin1String("BackupSystem"),
		 QLatin1String("Instance backups are now part of MeshMC itself "
					   "(instance settings -> Backups)")},
	};

	if (name.isEmpty())
		return nullptr;

	for (const auto& entry : entries) {
		if (name.compare(entry.moduleName, Qt::CaseInsensitive) == 0)
			return &entry;
	}
	return nullptr;
}
