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
#include "IconLabel.h"

#include <QStyle>
#include <QStyleOption>
#include <QLayout>
#include <QPainter>
#include <QRect>

IconLabel::IconLabel(QWidget* parent, QIcon icon, QSize size)
	: QWidget(parent), m_size(size), m_icon(icon)
{
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

QSize IconLabel::sizeHint() const
{
	return m_size;
}

void IconLabel::setIcon(QIcon icon)
{
	m_icon = icon;
	update();
}

void IconLabel::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	QRect rect = contentsRect();
	int width = rect.width();
	int height = rect.height();
	if (width < height) {
		rect.setHeight(width);
		rect.translate(0, (height - width) / 2);
	} else if (width > height) {
		rect.setWidth(height);
		rect.translate((width - height) / 2, 0);
	}
	m_icon.paint(&p, rect);
}
