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

#pragma once
#include <QString>
#include <QPalette>

class QStyle;

class ITheme
{
  public:
	virtual ~ITheme() {}
	virtual void apply(bool initial);
	virtual QString id() = 0;
	virtual QString name() = 0;
	virtual QString tooltip()
	{
		return QString();
	}
	virtual bool hasStyleSheet() = 0;
	virtual QString appStyleSheet() = 0;
	virtual QString qtTheme() = 0;
	virtual bool hasColorScheme() = 0;
	virtual QPalette colorScheme() = 0;
	virtual QColor fadeColor() = 0;
	virtual double fadeAmount() = 0;
	virtual QStringList searchPaths()
	{
		return {};
	}
	virtual QString family()
	{
		return name();
	}
	virtual QString variant()
	{
		return QString();
	}

	static QPalette fadeInactive(QPalette in, qreal bias, QColor color);
};
