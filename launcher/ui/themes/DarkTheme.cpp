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
	darkPalette.setColor(QPalette::Window, QColor(49, 49, 49));
	darkPalette.setColor(QPalette::WindowText, Qt::white);
	darkPalette.setColor(QPalette::Base, QColor(34, 34, 34));
	darkPalette.setColor(QPalette::AlternateBase, QColor(49, 49, 49));
	darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
	darkPalette.setColor(QPalette::ToolTipText, Qt::white);
	darkPalette.setColor(QPalette::Text, Qt::white);
	darkPalette.setColor(QPalette::Button, QColor(49, 49, 49));
	darkPalette.setColor(QPalette::ButtonText, Qt::white);
	darkPalette.setColor(QPalette::BrightText, Qt::red);
	darkPalette.setColor(QPalette::Link, QColor(47, 163, 198));
	darkPalette.setColor(QPalette::Highlight, QColor(150, 219, 89));
	darkPalette.setColor(QPalette::HighlightedText, Qt::black);
	darkPalette.setColor(QPalette::PlaceholderText, Qt::darkGray);
	return fadeInactive(darkPalette, fadeAmount(), fadeColor());
}

double DarkTheme::fadeAmount()
{
	return 0.5;
}

QColor DarkTheme::fadeColor()
{
	return QColor(49, 49, 49);
}

bool DarkTheme::hasStyleSheet()
{
	return true;
}

QString DarkTheme::appStyleSheet()
{
	return "QToolTip { color: #ffffff; background-color: #2fa3c6; border: 1px "
		   "solid white; }";
}
