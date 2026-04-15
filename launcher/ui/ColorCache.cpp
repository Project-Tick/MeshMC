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

#include "ColorCache.h"

/**
 * Blend the color with the front color, adapting to the back color
 */
QColor ColorCache::blend(QColor color)
{
	if (Rainbow::luma(m_front) > Rainbow::luma(m_back)) {
		// for dark color schemes, produce a fitting color first
		color = Rainbow::tint(m_front, color, 0.5);
	}
	// adapt contrast
	return Rainbow::mix(m_front, color, m_bias);
}

/**
 * Blend the color with the back color
 */
QColor ColorCache::blendBackground(QColor color)
{
	// adapt contrast
	return Rainbow::mix(m_back, color, m_bias);
}

void ColorCache::recolorAll()
{
	auto iter = m_colors.begin();
	while (iter != m_colors.end()) {
		iter->front = blend(iter->original);
		iter->back = blendBackground(iter->original);
	}
}
