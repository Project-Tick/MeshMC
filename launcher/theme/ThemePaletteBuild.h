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

#include <QColor>

#include "theme/ThemePalette.h"

/*
 * Internal to theme/: the half of every palette that is shared rather than
 * picked per scheme. The four named schemes (ThemePalette.cpp) and the
 * "system" scheme (SystemPalette.cpp) both describe themselves as a
 * SchemeColors and let build() derive the rest, so a translucent tint, a
 * hairline or a scrim is made the same way whichever of them is showing.
 *
 * Nothing outside theme/ includes this; QML and the UI read ThemePalette.
 */
namespace ThemePaletteBuild
{
	/* What differs between schemes. Everything else -- status colours,
	 * overlays, scrims -- is shared per mode, so switching scheme never
	 * changes what "danger" or "success" looks like. */
	struct SchemeColors
	{
		QColor canvas, surface, surfaceRaised, surfaceOverlay, surfaceSunken;
		QColor textPrimary, textSecondary, textTertiary, textDisabled;
		QColor accent, accentHover, accentPressed, accentText, textOnAccent;
		QColor borderStrong, selection, selectionText, tooltipBackground;
	};

	QColor withAlpha(QColor color, int alpha);

	/* The alpha every "Subtle" tint (accentSubtle, successSubtle...) gets
	 * over its own colour in that mode. */
	int subtleAlpha(bool dark);

	ThemePalette build(const SchemeColors& c, bool dark);
} // namespace ThemePaletteBuild
