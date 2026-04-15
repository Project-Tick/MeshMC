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

#include "MessageLevel.h"

MessageLevel::Enum MessageLevel::getLevel(const QString& levelName)
{
	if (levelName == "MeshMC")
		return MessageLevel::MeshMC;
	else if (levelName == "Debug")
		return MessageLevel::Debug;
	else if (levelName == "Info")
		return MessageLevel::Info;
	else if (levelName == "Message")
		return MessageLevel::Message;
	else if (levelName == "Warning")
		return MessageLevel::Warning;
	else if (levelName == "Error")
		return MessageLevel::Error;
	else if (levelName == "Fatal")
		return MessageLevel::Fatal;
	// Skip PrePost, it's not exposed to !![]!
	// Also skip StdErr and StdOut
	else
		return MessageLevel::Unknown;
}

MessageLevel::Enum MessageLevel::fromLine(QString& line)
{
	// Level prefix
	int endmark = line.indexOf("]!");
	if (line.startsWith("!![") && endmark != -1) {
		auto level = MessageLevel::getLevel(line.left(endmark).mid(3));
		line = line.mid(endmark + 2);
		return level;
	}
	return MessageLevel::Unknown;
}
