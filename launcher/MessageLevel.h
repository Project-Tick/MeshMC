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

/**
 * @brief the MessageLevel Enum
 * defines what level a log message is
 */
namespace MessageLevel
{
	enum Enum {
		Unknown, /**< No idea what this is or where it came from */
		StdOut,	 /**< Undetermined stderr messages */
		StdErr,	 /**< Undetermined stdout messages */
		MeshMC,	 /**< MeshMC Messages */
		Debug,	 /**< Debug Messages */
		Info,	 /**< Info Messages */
		Message, /**< Standard Messages */
		Warning, /**< Warnings */
		Error,	 /**< Errors */
		Fatal,	 /**< Fatal Errors */
	};
	MessageLevel::Enum getLevel(const QString& levelName);

	/* Get message level from a line. Line is modified if it was successful. */
	MessageLevel::Enum fromLine(QString& line);
} // namespace MessageLevel
