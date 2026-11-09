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


#include "WindowState.h"

#include "settings/KConfigSettingsObject.h"

#include <QByteArray>

#include <KConfig>
#include <KConfigGroup>
#include <KMainWindow>

namespace WindowState
{
	void setup(KMainWindow* window, const QString& group,
			   KConfigSettingsObject* legacy, const QString& geometryKey,
			   const QString& stateKey)
	{
		// Window size and state go to a group of their own: KMainWindow uses
		// "MainWindow" for all of its windows unless it is told otherwise,
		// which would make the console window start like the main window.
		// Has to come first.
		window->setStateConfigGroup(group);

		// Whether the window saved anything of its own, which is asked before
		// setAutoSaveSettings() makes the group.
		const bool saved = window->stateConfigGroup().exists();
		window->setAutoSaveSettings(group, true);

		QByteArray legacyGeometry;
		QByteArray legacyState;
		if (legacy) {
			KConfigGroup top = legacy->config()->group(QString());
			legacyGeometry = QByteArray::fromBase64(
				top.readEntry(geometryKey, QString()).toUtf8());
			legacyState = QByteArray::fromBase64(
				top.readEntry(stateKey, QString()).toUtf8());

			if (top.hasKey(geometryKey) || top.hasKey(stateKey)) {
				top.deleteEntry(geometryKey);
				top.deleteEntry(stateKey);
				legacy->config()->sync();
			}
		}

		if (saved) {
			window->applyMainWindowSettings(window->autoSaveConfigGroup());
			return;
		}

		// The state first, then the geometry, as it always was.
		if (!legacyState.isEmpty()) {
			window->restoreState(legacyState);
		}
		if (!legacyGeometry.isEmpty()) {
			window->restoreGeometry(legacyGeometry);
		}
	}
} // namespace WindowState
