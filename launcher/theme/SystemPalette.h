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

class QPalette;

/*
 * The desktop's own colour scheme, as the "system" palette reads it: only
 * the colours a desktop actually defines, before ThemePalette::fromSystem()
 * fits them to the design's roles.
 *
 * Two places fill it in. fromPalette() reads a QPalette -- what macOS,
 * Windows and non-KDE Linux desktops hand every Qt application. On a Plasma
 * session ThemeService reads Kirigami's platform theme instead (see
 * KirigamiColors), which has the colour sets and the positive/neutral/
 * negative colours a QPalette has no room for.
 *
 * An invalid QColor means "this source has no such colour"; fromSystem()
 * derives that role itself. Colours may be translucent (macOS gives its
 * text colours an alpha); fromSystem() flattens them onto what they sit on.
 */
struct SystemColors
{
	QColor window; // the window's own background
	QColor view;   // a content view's (QPalette::Base, Kirigami's View set)
	QColor text;   // text on the window
	QColor placeholderText;
	QColor disabledText;

	QColor accent;
	/* The label the scheme itself puts on `accent` as a fill, if it pairs
	 * one with it. A QPalette's HighlightedText belongs to Highlight, which
	 * is only the accent where the two are the same colour -- not on macOS,
	 * whose Highlight is the pale selected-text tint -- so fromPalette()
	 * leaves this invalid there. */
	QColor accentLabel;

	QColor highlight; // selected text's background
	QColor highlightedText;
	QColor focus;
	QColor tooltip;
	QColor tooltipText;

	// Only Kirigami has these; a QPalette leaves them invalid.
	QColor positive;
	QColor neutral;
	QColor negative;
	QColor info;

	static SystemColors fromPalette(const QPalette& palette);

	/* Whether this is a dark scheme: the window's lightness under half, the
	 * same test ThemeService's "system" mode has always used. */
	bool isDark() const;
};
