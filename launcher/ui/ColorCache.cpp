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
