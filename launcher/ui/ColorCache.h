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

#pragma once
#include <QtGui/QColor>
#include <rainbow.h>
#include <MessageLevel.h>
#include <QMap>

class ColorCache
{
  public:
	ColorCache(QColor front, QColor back, qreal bias)
	{
		m_front = front;
		m_back = back;
		m_bias = bias;
	};

	void addColor(int key, QColor color)
	{
		m_colors[key] = {color, blend(color), blendBackground(color)};
	}

	void setForeground(QColor front)
	{
		if (m_front != front) {
			m_front = front;
			recolorAll();
		}
	}

	void setBackground(QColor back)
	{
		if (m_back != back) {
			m_back = back;
			recolorAll();
		}
	}

	QColor getFront(int key)
	{
		auto iter = m_colors.find(key);
		if (iter == m_colors.end()) {
			return QColor();
		}
		return (*iter).front;
	}

	QColor getBack(int key)
	{
		auto iter = m_colors.find(key);
		if (iter == m_colors.end()) {
			return QColor();
		}
		return (*iter).back;
	}

	/**
	 * Blend the color with the front color, adapting to the back color
	 */
	QColor blend(QColor color);

	/**
	 * Blend the color with the back color
	 */
	QColor blendBackground(QColor color);

  protected:
	void recolorAll();

  protected:
	struct ColorEntry {
		QColor original;
		QColor front;
		QColor back;
	};

  protected:
	qreal m_bias;
	QColor m_front;
	QColor m_back;
	QMap<int, ColorEntry> m_colors;
};

class LogColorCache : public ColorCache
{
  public:
	LogColorCache(QColor front, QColor back) : ColorCache(front, back, 1.0)
	{
		addColor((int)MessageLevel::MeshMC, QColor("purple"));
		addColor((int)MessageLevel::Debug, QColor("green"));
		addColor((int)MessageLevel::Warning, QColor("orange"));
		addColor((int)MessageLevel::Error, QColor("red"));
		addColor((int)MessageLevel::Fatal, QColor("red"));
		addColor((int)MessageLevel::Message, front);
	}

	QColor getFront(MessageLevel::Enum level)
	{
		if (!m_colors.contains((int)level)) {
			return ColorCache::getFront((int)MessageLevel::Message);
		}
		return ColorCache::getFront((int)level);
	}

	QColor getBack(MessageLevel::Enum level)
	{
		if (level == MessageLevel::Fatal) {
			return QColor(Qt::black);
		}
		return QColor(Qt::transparent);
	}
};
