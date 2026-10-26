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
