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

#include "KonamiCode.h"

#include <array>
#include <QDebug>

namespace
{
	const std::array<Qt::Key, 10> konamiCode = {
		{Qt::Key_Up, Qt::Key_Up, Qt::Key_Down, Qt::Key_Down, Qt::Key_Left,
		 Qt::Key_Right, Qt::Key_Left, Qt::Key_Right, Qt::Key_B, Qt::Key_A}};
}

KonamiCode::KonamiCode(QObject* parent) : QObject(parent) {}

void KonamiCode::input(QEvent* event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		auto key = Qt::Key(keyEvent->key());
		if (key == konamiCode[m_progress]) {
			m_progress++;
		} else {
			m_progress = 0;
		}
		if (m_progress == static_cast<int>(konamiCode.size())) {
			m_progress = 0;
			emit triggered();
		}
	}
}
