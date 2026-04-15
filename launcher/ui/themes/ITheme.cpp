/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 */

#include "ITheme.h"
#include "rainbow.h"
#include <QStyleFactory>
#include <QDir>
#include "Application.h"

void ITheme::apply(bool)
{
	APPLICATION->setStyleSheet(QString());
	QApplication::setStyle(QStyleFactory::create(qtTheme()));
	QApplication::setPalette(colorScheme());
	APPLICATION->setStyleSheet(appStyleSheet());
	QDir::setSearchPaths("theme", searchPaths());
}

QPalette ITheme::fadeInactive(QPalette in, qreal bias, QColor color)
{
	auto blend = [&in, bias, color](QPalette::ColorRole role) {
		QColor from = in.color(QPalette::Active, role);
		QColor blended = Rainbow::mix(from, color, bias);
		in.setColor(QPalette::Disabled, role, blended);
	};
	blend(QPalette::Window);
	blend(QPalette::WindowText);
	blend(QPalette::Base);
	blend(QPalette::AlternateBase);
	blend(QPalette::ToolTipBase);
	blend(QPalette::ToolTipText);
	blend(QPalette::Text);
	blend(QPalette::Button);
	blend(QPalette::ButtonText);
	blend(QPalette::BrightText);
	blend(QPalette::Link);
	blend(QPalette::Highlight);
	blend(QPalette::HighlightedText);
	return in;
}
