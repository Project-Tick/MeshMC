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
