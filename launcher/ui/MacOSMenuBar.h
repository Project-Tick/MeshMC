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

#pragma once

class QAction;
class QMainWindow;
class QMenu;
class QMenuBar;

/// Installs a native macOS top menu bar built from MainWindow's existing
/// QActions. Toggled by the `UseMacNativeMenuBar` setting; no-op off macOS.
class MacOSMenuBar
{
  public:
	struct Actions {
		// Instance lifecycle
		QAction* addInstance = nullptr;
		QAction* launch = nullptr;
		QAction* launchOffline = nullptr;
		QAction* editInstance = nullptr;
		QAction* instanceSettings = nullptr;
		QAction* editNotes = nullptr;
		QAction* viewMods = nullptr;
		QAction* viewWorlds = nullptr;
		QAction* screenshots = nullptr;
		QAction* rename = nullptr;
		QAction* changeGroup = nullptr;
		QAction* changeIcon = nullptr;
		QAction* copyInstance = nullptr;
		QAction* exportInstance = nullptr;
		QAction* deleteInstance = nullptr;

		// Folders
		QAction* viewInstanceFolder = nullptr;
		QAction* viewCentralModsFolder = nullptr;
		QAction* viewSelectedMCFolder = nullptr;
		QAction* viewSelectedModsFolder = nullptr;
		QAction* viewSelectedConfigFolder = nullptr;
		QAction* viewSelectedInstFolder = nullptr;

		// View
		QAction* toggleCat = nullptr;

		// App-menu (auto-relocated via menuRole)
		QAction* preferences = nullptr; // PreferencesRole
		QAction* about = nullptr;		// AboutRole

		// Accounts
		QAction* manageAccounts = nullptr;
		QMenu* accountSubmenu = nullptr; // dynamic account list, may be null

		// Help & updates
		QAction* reportBug = nullptr;
		QAction* discord = nullptr;
		QAction* reddit = nullptr;
		QAction* plugins = nullptr;
		QAction* viewLogs = nullptr;
		QAction* checkUpdate = nullptr;
	};

	/**
	 * Build a QMenuBar from the supplied actions and attach it to @p window.
	 *
	 * Returns the created QMenuBar (owned by @p window) on macOS when the
	 * `UseMacNativeMenuBar` setting is true. Returns nullptr on every other
	 * platform or when the user disabled the top bar.
	 */
	static QMenuBar* install(QMainWindow* window, const Actions& actions);

	/// Application-settings key used to toggle the native top bar.
	static constexpr const char* kSettingKey = "UseMacNativeMenuBar";
};
