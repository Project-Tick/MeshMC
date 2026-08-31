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

#include "GreenDarkTheme.h"

#include <QObject>

QString GreenDarkTheme::id()
{
	return "green-dark";
}

QString GreenDarkTheme::name()
{
	return QObject::tr("Green Dark");
}

QString GreenDarkTheme::tooltip()
{
	return QObject::tr("A dark Fusion-based theme with green accents");
}

bool GreenDarkTheme::hasColorScheme()
{
	return true;
}

QPalette GreenDarkTheme::colorScheme()
{
	QPalette palette;
	palette.setColor(QPalette::Window, QColor(49, 49, 49));
	palette.setColor(QPalette::WindowText, Qt::white);
	palette.setColor(QPalette::Base, QColor(34, 34, 34));
	palette.setColor(QPalette::AlternateBase, QColor(42, 42, 42));
	palette.setColor(QPalette::ToolTipBase, Qt::white);
	palette.setColor(QPalette::ToolTipText, Qt::white);
	palette.setColor(QPalette::Text, Qt::white);
	palette.setColor(QPalette::Button, QColor(48, 48, 48));
	palette.setColor(QPalette::ButtonText, Qt::white);
	palette.setColor(QPalette::BrightText, Qt::red);
	palette.setColor(QPalette::Link, QColor(47, 163, 198));
	palette.setColor(QPalette::Highlight, QColor(150, 219, 89));
	palette.setColor(QPalette::HighlightedText, Qt::black);
	palette.setColor(QPalette::PlaceholderText, Qt::darkGray);
	return fadeInactive(palette, fadeAmount(), fadeColor());
}

double GreenDarkTheme::fadeAmount()
{
	return 0.5;
}

QColor GreenDarkTheme::fadeColor()
{
	return QColor(49, 49, 49);
}

bool GreenDarkTheme::hasStyleSheet()
{
	return true;
}

QString GreenDarkTheme::appStyleSheet()
{
	return "QToolTip { color: #ffffff; background-color: #2fa3c6; border: 1px "
		   "solid white; }";
}
