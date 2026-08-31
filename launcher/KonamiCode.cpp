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

#include "KonamiCode.h"

#include <array>
#include <QDebug>

namespace
{
	const std::array<Qt::Key, 10> konamiCode = {
		{Qt::Key_Up, Qt::Key_Up, Qt::Key_Down, Qt::Key_Down, Qt::Key_Left,
		 Qt::Key_Right, Qt::Key_Left, Qt::Key_Right, Qt::Key_B, Qt::Key_A}};
}

KonamiCode::KonamiCode(QObject* parent) : QObject(parent) {}

void KonamiCode::input(QEvent* event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		auto key = Qt::Key(keyEvent->key());
		if (key == konamiCode[m_progress]) {
			m_progress++;
		} else {
			m_progress = 0;
		}
		if (m_progress == static_cast<int>(konamiCode.size())) {
			m_progress = 0;
			emit triggered();
		}
	}
}
