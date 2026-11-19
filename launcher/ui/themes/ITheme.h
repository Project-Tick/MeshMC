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
