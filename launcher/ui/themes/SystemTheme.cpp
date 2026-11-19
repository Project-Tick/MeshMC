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
