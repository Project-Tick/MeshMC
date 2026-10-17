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

#include "qml/Theme/ThemeService.h"

#include <QDebug>
#include <QEvent>
#include <QGuiApplication>
#include <QPalette>

#include "qml/Theme/KirigamiColors.h"

namespace
{
	const QString kSystemScheme = QStringLiteral("system");
} // namespace

ThemeService::ThemeService(QObject* parent)
	: QObject(parent), m_kirigami(KirigamiColors::forDesktop())
{
	if (m_kirigami)
		connect(m_kirigami.get(), &KirigamiColors::changed, this,
				&ThemeService::readSystemColors);
	/* On the application rather than one window, so it is there before any
	 * window is and does not depend on which one the event reaches first.
	 * eventFilter() looks at the event type before anything else. */
	qGuiApp->installEventFilter(this);

	readSystemColors();
	resolve(false);
}

ThemeService::~ThemeService() = default;

QString ThemeService::scheme() const
{
	return m_followSystem ? kSystemScheme : ThemePalette::schemeName(m_scheme);
}

void ThemeService::setScheme(const QString& scheme)
{
	if (scheme == kSystemScheme) {
		if (m_followSystem)
			return;
		m_followSystem = true;
		resolve(true);
		return;
	}

	const ThemePalette::Scheme parsed = ThemePalette::schemeFromName(scheme);
	if (ThemePalette::schemeName(parsed) != scheme) {
		qWarning() << "ThemeService: ignoring unknown colour scheme" << scheme;
		return;
	}
	if (!m_followSystem && parsed == m_scheme)
		return;
	m_followSystem = false;
	m_scheme = parsed;
	resolve(true);
}

ThemePalette ThemeService::previewPalette(const QString& scheme) const
{
	if (scheme == kSystemScheme)
		return m_systemPalette;
	return ThemePalette::forScheme(ThemePalette::schemeFromName(scheme), modeIsDark());
}

void ThemeService::setMode(const QString& mode)
{
	if (mode != QStringLiteral("system") && mode != QStringLiteral("dark") &&
		mode != QStringLiteral("light")) {
		qWarning() << "ThemeService: ignoring unknown theme mode" << mode;
		return;
	}
	if (mode == m_mode)
		return;

	m_mode = mode;
	// The mode itself changed either way, so the whole theme is announced as
	// one unit even on the rare switch that leaves `dark` unchanged (e.g.
	// "system" -> "light" while the system is already light).
	resolve(true);
}

bool ThemeService::eventFilter(QObject* watched, QEvent* event)
{
	/* Qt passes the event on to every window after the application, so only
	 * the application's own copy is acted on. */
	if (event->type() == QEvent::ApplicationPaletteChange && watched == qGuiApp)
		readSystemColors();
	return QObject::eventFilter(watched, event);
}

void ThemeService::readSystemColors()
{
	const SystemColors colors =
		m_kirigami ? m_kirigami->colors() : SystemColors::fromPalette(qGuiApp->palette());
	const ThemePalette palette = ThemePalette::fromSystem(colors);
	const bool dark = colors.isDark();
	if (palette == m_systemPalette && dark == m_systemDark)
		return;

	m_systemPalette = palette;
	m_systemDark = dark;
	emit systemPaletteChanged();
	resolve(false);
}

bool ThemeService::modeIsDark() const
{
	if (m_mode == QStringLiteral("system"))
		return m_systemDark;
	return m_mode == QStringLiteral("dark");
}

void ThemeService::resolve(bool announce)
{
	const bool dark = m_followSystem ? m_systemDark : modeIsDark();
	const ThemePalette palette =
		m_followSystem ? m_systemPalette : ThemePalette::forScheme(m_scheme, dark);
	if (!announce && dark == m_dark && palette == m_palette)
		return;

	m_dark = dark;
	m_palette = palette;
	emit changed();
}
