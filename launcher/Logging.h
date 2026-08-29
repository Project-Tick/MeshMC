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

#include <QLoggingCategory>
#include <QString>

/*
 * Logging categories for the launcher's own output.
 *
 * A category does two things for us: it tags the line in the log, so a reader
 * can tell our noise apart from Qt's, and it gives users a filter they can
 * aim, for example
 *
 *     QT_LOGGING_RULES="meshmc.net.debug=false"
 *
 * to silence one subsystem without losing its warnings.
 *
 * To add one: declare it here, define it in Logging.cpp with the matching
 * "meshmc.<subsystem>" string, then use qCDebug(<name>) instead of qDebug()
 * in that subsystem. Uncategorised qDebug() keeps working and keeps printing
 * exactly as it did - Qt reports its category as "default", which the log
 * layout treats as "no category" and omits.
 *
 * NOTE: only declare categories that are actually in use. An unused category
 * is a promise about where messages come from that nothing keeps.
 */

/// Downloads, HTTP caching, network jobs.
Q_DECLARE_LOGGING_CATEGORY(netLog)

namespace Logging
{
	/**
	 * @brief the launcher's log line layout, as a Qt message pattern
	 *
	 * Handed to qSetMessagePattern() so that Qt's own formatter does the
	 * work; that is what makes %{category} and a readable %{function}
	 * available at all.
	 *
	 * @param coloured whether to wrap the level letter in ANSI colour, which
	 *                 is wanted for the console copy but not for the log file
	 * @param withSourceLocation whether to append "(function:line)"
	 *
	 * withSourceLocation is a per-message decision, not a per-build one: Qt's
	 * own libraries are built without QT_MESSAGELOGCONTEXT, so anything
	 * raised inside Qt arrives with an empty context and must be rendered
	 * without the suffix, or every such line ends in a useless "(unknown:0)".
	 * The caller decides by looking at QMessageLogContext::line.
	 */
	QString messagePattern(bool coloured, bool withSourceLocation);

	/**
	 * @brief make the console able to render ANSI colour, and report whether
	 *        it now can
	 *
	 * Call once, early, and after the console has been attached and the
	 * standard streams reopened - the answer depends on what stderr actually
	 * points at.
	 *
	 * This is not a formality on Windows: a console does not interpret escape
	 * sequences until someone sets ENABLE_VIRTUAL_TERMINAL_PROCESSING on it,
	 * and the classic console host starts with that bit clear. Printing
	 * colour anyway is how a log line ends up reading "<ESC>[32mD:<ESC>[0m".
	 */
	bool prepareConsoleColour();

	/// Whether the console copy of a log line may carry ANSI colour. False
	/// until prepareConsoleColour() has said otherwise.
	bool consoleColourEnabled();
} // namespace Logging
