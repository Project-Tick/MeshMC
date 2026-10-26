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

#include "BrightTheme.h"

#include <QObject>

QString BrightTheme::id()
{
	return "bright";
}

QString BrightTheme::name()
{
	return QObject::tr("Bright");
}

QString BrightTheme::tooltip()
{
	return QObject::tr("A bright Fusion-based theme with blue accents");
}

bool BrightTheme::hasColorScheme()
{
	return true;
}

QPalette BrightTheme::colorScheme()
{
	QPalette brightPalette;
	brightPalette.setColor(QPalette::Window, QColor(239, 240, 241));
	brightPalette.setColor(QPalette::WindowText, QColor(49, 54, 59));
	brightPalette.setColor(QPalette::Base, QColor(252, 252, 252));
	brightPalette.setColor(QPalette::AlternateBase, QColor(239, 240, 241));
	brightPalette.setColor(QPalette::ToolTipBase, QColor(49, 54, 59));
	brightPalette.setColor(QPalette::ToolTipText, QColor(239, 240, 241));
	brightPalette.setColor(QPalette::Text, QColor(49, 54, 59));
	brightPalette.setColor(QPalette::Button, QColor(239, 240, 241));
	brightPalette.setColor(QPalette::ButtonText, QColor(49, 54, 59));
	brightPalette.setColor(QPalette::BrightText, Qt::red);
	brightPalette.setColor(QPalette::Link, QColor(41, 128, 185));
	brightPalette.setColor(QPalette::Highlight, QColor(61, 174, 233));
	brightPalette.setColor(QPalette::HighlightedText, QColor(239, 240, 241));
	return fadeInactive(brightPalette, fadeAmount(), fadeColor());
}

double BrightTheme::fadeAmount()
{
	return 0.5;
}

QColor BrightTheme::fadeColor()
{
	return QColor(239, 240, 241);
}

bool BrightTheme::hasStyleSheet()
{
	return false;
}

QString BrightTheme::appStyleSheet()
{
	return QString();
}
