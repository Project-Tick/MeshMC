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

#include "TextPrint.h"

TextPrint::TextPrint(LaunchTask* parent, const QStringList& lines,
					 MessageLevel::Enum level)
	: LaunchStep(parent)
{
	m_lines = lines;
	m_level = level;
}
TextPrint::TextPrint(LaunchTask* parent, const QString& line,
					 MessageLevel::Enum level)
	: LaunchStep(parent)
{
	m_lines.append(line);
	m_level = level;
}

void TextPrint::executeTask()
{
	emit logLines(m_lines, m_level);
	emitSucceeded();
}

bool TextPrint::canAbort() const
{
	return true;
}

bool TextPrint::abort()
{
	emitFailed("Aborted.");
	return true;
}
