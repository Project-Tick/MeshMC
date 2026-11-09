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

class KMainWindow;
class KConfigSettingsObject;

/*!
 * How a main window of the application remembers where it was and what it
 * looked like, by way of KMainWindow.
 */
namespace WindowState
{
	/*!
	 * Makes \p window keep its size, its position and the layout of its
	 * toolbars under \p group of KDE's state configuration, and puts it back
	 * the way it was.
	 *
	 * The window has to be finished (its toolbars made), and not shown yet.
	 * It saves itself when KMainWindow::saveAutoSaveSettings() is called, which
	 * its closeEvent() does: KMainWindow::closeEvent() would do that too,
	 * but it also quits the application when it is the last window, which
	 * MeshMC decides for itself.
	 *
	 * Earlier versions kept geometry and state in the settings, as the text
	 * saveGeometry() and saveState() make. If \p legacy has those (\p
	 * geometryKey and \p stateKey), and the window has not saved anything of its
	 * own yet, it takes them, once. They are removed from \p legacy either way.
	 */
	void setup(KMainWindow* window, const QString& group,
			   KConfigSettingsObject* legacy, const QString& geometryKey,
			   const QString& stateKey);
} // namespace WindowState
