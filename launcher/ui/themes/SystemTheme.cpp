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

#include "SystemTheme.h"
#include <QApplication>
#include <QStyle>
#include <QStyleFactory>
#include <QDebug>

static const QStringList S_NATIVE_STYLES{"windows11", "windowsvista", "macos",
										 "system", "windows"};

SystemTheme::SystemTheme(const QString& styleName,
						 const QPalette& defaultPalette, bool isDefaultTheme)
{
	m_themeName = isDefaultTheme ? "system" : styleName;
	m_widgetTheme = styleName;
	if (S_NATIVE_STYLES.contains(m_themeName)) {
		m_colorPalette = defaultPalette;
	} else {
		// If this style matches the system's current default style, use the
		// application palette instead of standardPalette().  standardPalette()
		// returns a hardcoded palette that ignores the platform color scheme
		// (e.g. Breeze on Plasma always returns a dark standardPalette even
		// when the system is set to a light color scheme).
		auto currentDefault = QApplication::style()->objectName();
		if (styleName.compare(currentDefault, Qt::CaseInsensitive) == 0) {
			m_colorPalette = defaultPalette;
		} else {
			auto style = QStyleFactory::create(styleName);
			m_colorPalette =
				style != nullptr ? style->standardPalette() : defaultPalette;
			delete style;
		}
	}
}

void SystemTheme::apply(bool initial)
{
	if (initial && S_NATIVE_STYLES.contains(m_themeName)) {
		return;
	}
	ITheme::apply(initial);
}

QString SystemTheme::id()
{
	return m_themeName;
}

QString SystemTheme::name()
{
	if (m_themeName.toLower() == "windowsvista") {
		return QObject::tr("Windows Vista");
	} else if (m_themeName.toLower() == "windows") {
		return QObject::tr("Windows 9x");
	} else if (m_themeName.toLower() == "windows11") {
		return QObject::tr("Windows 11");
	} else if (m_themeName.toLower() == "system") {
		return QObject::tr("System");
	} else {
		return m_themeName;
	}
}

QString SystemTheme::tooltip()
{
	if (m_themeName.toLower() == "windowsvista") {
		return QObject::tr("Widget style trying to look like your win32 theme");
	} else if (m_themeName.toLower() == "windows") {
		return QObject::tr("Windows 9x inspired widget style");
	} else if (m_themeName.toLower() == "windows11") {
		return QObject::tr("WinUI 3 inspired Qt widget style");
	} else if (m_themeName.toLower() == "fusion") {
		return QObject::tr("The default Qt widget style");
	} else if (m_themeName.toLower() == "system") {
		return QObject::tr("Your current system theme");
	} else {
		return QString();
	}
}

QString SystemTheme::qtTheme()
{
	return m_widgetTheme;
}

QPalette SystemTheme::colorScheme()
{
	return m_colorPalette;
}

QString SystemTheme::appStyleSheet()
{
	return QString();
}

double SystemTheme::fadeAmount()
{
	return 0.5;
}

QColor SystemTheme::fadeColor()
{
	return QColor(128, 128, 128);
}

bool SystemTheme::hasStyleSheet()
{
	return false;
}

bool SystemTheme::hasColorScheme()
{
	return true;
}
