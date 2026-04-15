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

#include "LineSeparator.h"

#include <QStyle>
#include <QStyleOption>
#include <QLayout>
#include <QPainter>

void LineSeparator::initStyleOption(QStyleOption* option) const
{
	option->initFrom(this);
	// in a horizontal layout, the line is vertical (and vice versa)
	if (m_orientation == Qt::Vertical)
		option->state |= QStyle::State_Horizontal;
}

LineSeparator::LineSeparator(QWidget* parent, Qt::Orientation orientation)
	: QWidget(parent), m_orientation(orientation)
{
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

QSize LineSeparator::sizeHint() const
{
	QStyleOption opt;
	initStyleOption(&opt);
	const int extent = style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent,
											&opt, parentWidget());
	return QSize(extent, extent);
}

void LineSeparator::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	QStyleOption opt;
	initStyleOption(&opt);
	style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, &p,
						   parentWidget());
}
