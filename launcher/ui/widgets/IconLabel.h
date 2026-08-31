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

#pragma once
#include <QWidget>
#include <QIcon>

class QStyleOption;

/**
 * This is a trivial widget that paints a QIcon of the specified size.
 */
class IconLabel : public QWidget
{
	Q_OBJECT

  public:
	/// Create a line separator. orientation is the orientation of the line.
	explicit IconLabel(QWidget* parent, QIcon icon, QSize size);

	virtual QSize sizeHint() const;
	virtual void paintEvent(QPaintEvent*);

	void setIcon(QIcon icon);

  private:
	QSize m_size;
	QIcon m_icon;
};
