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

#include "DarkTheme.h"

#include <QObject>

QString DarkTheme::id()
{
	return "dark";
}

QString DarkTheme::name()
{
	return QObject::tr("Dark");
}

QString DarkTheme::tooltip()
{
	return QObject::tr("A dark Fusion-based theme with green accents");
}

bool DarkTheme::hasColorScheme()
{
	return true;
}

QPalette DarkTheme::colorScheme()
{
	QPalette darkPalette;
	darkPalette.setColor(QPalette::Window, QColor(49, 54, 59));
	darkPalette.setColor(QPalette::WindowText, Qt::white);
	darkPalette.setColor(QPalette::Base, QColor(35, 38, 41));
	darkPalette.setColor(QPalette::AlternateBase, QColor(49, 54, 59));
	darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
	darkPalette.setColor(QPalette::ToolTipText, Qt::white);
	darkPalette.setColor(QPalette::Text, Qt::white);
	darkPalette.setColor(QPalette::Button, QColor(49, 54, 59));
	darkPalette.setColor(QPalette::ButtonText, Qt::white);
	darkPalette.setColor(QPalette::BrightText, Qt::red);
	darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::HighlightedText, Qt::black);
	return fadeInactive(darkPalette, fadeAmount(), fadeColor());
}

double DarkTheme::fadeAmount()
{
	return 0.5;
}

QColor DarkTheme::fadeColor()
{
	return QColor(49, 54, 59);
}

bool DarkTheme::hasStyleSheet()
{
	return true;
}

QString DarkTheme::appStyleSheet()
{
	return "QToolTip { color: #ffffff; background-color: #2a82da; border: 1px "
		   "solid white; }";
}
