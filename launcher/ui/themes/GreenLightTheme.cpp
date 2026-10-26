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

#include "GreenLightTheme.h"

#include <QObject>

QString GreenLightTheme::id()
{
	return "green-light";
}

QString GreenLightTheme::name()
{
	return QObject::tr("Green Light");
}

QString GreenLightTheme::tooltip()
{
	return QObject::tr("A bright Fusion-based theme with green accents");
}

bool GreenLightTheme::hasColorScheme()
{
	return true;
}

QPalette GreenLightTheme::colorScheme()
{
	QPalette palette;
	palette.setColor(QPalette::Window, QColor(255, 255, 255));
	palette.setColor(QPalette::WindowText, QColor(49, 49, 49));
	palette.setColor(QPalette::Base, QColor(250, 250, 250));
	palette.setColor(QPalette::AlternateBase, QColor(239, 240, 241));
	palette.setColor(QPalette::ToolTipBase, QColor(49, 49, 49));
	palette.setColor(QPalette::ToolTipText, QColor(239, 240, 241));
	palette.setColor(QPalette::Text, QColor(49, 49, 49));
	palette.setColor(QPalette::Button, QColor(255, 255, 255));
	palette.setColor(QPalette::ButtonText, QColor(49, 49, 49));
	palette.setColor(QPalette::BrightText, Qt::red);
	palette.setColor(QPalette::Link, QColor(37, 137, 164));
	palette.setColor(QPalette::Highlight, QColor(137, 207, 84));
	palette.setColor(QPalette::HighlightedText, QColor(239, 240, 241));
	return fadeInactive(palette, fadeAmount(), fadeColor());
}

double GreenLightTheme::fadeAmount()
{
	return 0.5;
}

QColor GreenLightTheme::fadeColor()
{
	return QColor(255, 255, 255);
}

bool GreenLightTheme::hasStyleSheet()
{
	return false;
}

QString GreenLightTheme::appStyleSheet()
{
	return QString();
}
