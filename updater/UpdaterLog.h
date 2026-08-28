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

#include <QString>

/*!
 * The updater's log file.
 *
 * On Windows the updater is a GUI-subsystem binary, so it has no console:
 * qDebug(), qCritical() and fprintf(stderr) all go nowhere, and a failed
 * update is indistinguishable from a successful one. Everything the updater
 * says therefore goes to a file, from the first line onwards.
 *
 * Both stages append to the same file so a whole update reads as one story.
 * The previous run is kept alongside it, because the interesting failure is
 * usually the one before the user thought to look.
 */
namespace UpdaterLog
{

	/*!
	 * Start logging. Call once, as early as possible.
	 *
	 * \a dataDir is where the log belongs; if it cannot be written the
	 * temporary directory is used instead, so there is always somewhere to
	 * look. \a rotate should be true for the first stage of an update and false
	 * for the second, which continues the same story.
	 *
	 * \a mirrorToConsole additionally writes to the console that started the
	 * process, when there is one -- see attachToParentConsole().
	 */
	void start(const QString& dataDir, bool rotate, bool mirrorToConsole);

	//! Where start() ended up writing. Shown to the user when an update fails.
	QString filePath();

	//! Flush and uninstall the handler. Called from main() on the way out.
	void stop();

	/*!
	 * Windows only: adopt the console of the process that started us, so a
	 * developer running the updater by hand actually sees its output. Does
	 * nothing when there is no parent console, and nothing at all elsewhere.
	 */
	bool attachToParentConsole();

} // namespace UpdaterLog
