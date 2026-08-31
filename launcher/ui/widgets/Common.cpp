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

#include "Common.h"

// Origin: Qt
QStringList viewItemTextLayout(QTextLayout& textLayout, int lineWidth,
							   qreal& height, qreal& widthUsed)
{
	QStringList lines;
	height = 0;
	widthUsed = 0;
	textLayout.beginLayout();
	QString str = textLayout.text();
	while (true) {
		QTextLine line = textLayout.createLine();
		if (!line.isValid())
			break;
		if (line.textLength() == 0)
			break;
		line.setLineWidth(lineWidth);
		line.setPosition(QPointF(0, height));
		height += line.height();
		lines.append(str.mid(line.textStart(), line.textLength()));
		widthUsed = qMax(widthUsed, line.naturalTextWidth());
	}
	textLayout.endLayout();
	return lines;
}
