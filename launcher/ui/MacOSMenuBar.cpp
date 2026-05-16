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
 */

#include "MacOSMenuBar.h"

#include <QAction>
#include <QCoreApplication>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QtGlobal>

#ifdef Q_OS_MACOS
#include "Application.h"
#include "settings/SettingsObject.h"
#endif

namespace
{
inline void addIfPresent(QMenu* menu, QAction* action)
{
	if (menu && action) {
		menu->addAction(action);
	}
}

#ifdef Q_OS_MACOS
bool useNativeMenuBar()
{
	if (!APPLICATION || !APPLICATION->settings()) {
		return true;
	}
	return APPLICATION->settings()
		->get(MacOSMenuBar::kSettingKey)
		.toBool();
}
#endif
}  // namespace

QMenuBar* MacOSMenuBar::install(QMainWindow* window, const Actions& actions)
{
#ifndef Q_OS_MACOS
	Q_UNUSED(window);
	Q_UNUSED(actions);
	return nullptr;
#else
	if (!window) {
		return nullptr;
	}
	if (!useNativeMenuBar()) {
		return nullptr;
	}

	// Parent the menu bar to the main window so it's part of the window's
	// object tree and gets destroyed with it. Qt still hoists the items into
	// the system-wide menu bar at the top of the screen because nativeMenuBar
	// is the macOS default.
	auto* bar = new QMenuBar(window);
	bar->setNativeMenuBar(true);

	// === File =============================================================
	auto* fileMenu =
		bar->addMenu(QCoreApplication::translate("MacOSMenuBar", "&File"));
	addIfPresent(fileMenu, actions.addInstance);
	if (actions.launch || actions.launchOffline) {
		fileMenu->addSeparator();
		addIfPresent(fileMenu, actions.launch);
		addIfPresent(fileMenu, actions.launchOffline);
	}
	if (actions.copyInstance || actions.exportInstance) {
		fileMenu->addSeparator();
		addIfPresent(fileMenu, actions.copyInstance);
		addIfPresent(fileMenu, actions.exportInstance);
	}
	if (actions.deleteInstance) {
		fileMenu->addSeparator();
		fileMenu->addAction(actions.deleteInstance);
	}
	// Qt auto-relocates PreferencesRole into the app menu on macOS.
	if (actions.preferences) {
		fileMenu->addSeparator();
		fileMenu->addAction(actions.preferences);
	}

	// === Instance =========================================================
	auto* instanceMenu =
		bar->addMenu(QCoreApplication::translate("MacOSMenuBar", "&Instance"));
	addIfPresent(instanceMenu, actions.editInstance);
	addIfPresent(instanceMenu, actions.instanceSettings);
	addIfPresent(instanceMenu, actions.editNotes);
	if (actions.viewMods || actions.viewWorlds || actions.screenshots) {
		instanceMenu->addSeparator();
		addIfPresent(instanceMenu, actions.viewMods);
		addIfPresent(instanceMenu, actions.viewWorlds);
		addIfPresent(instanceMenu, actions.screenshots);
	}
	if (actions.rename || actions.changeIcon || actions.changeGroup) {
		instanceMenu->addSeparator();
		addIfPresent(instanceMenu, actions.rename);
		addIfPresent(instanceMenu, actions.changeIcon);
		addIfPresent(instanceMenu, actions.changeGroup);
	}

	// === Folders ==========================================================
	auto* foldersMenu =
		bar->addMenu(QCoreApplication::translate("MacOSMenuBar", "F&olders"));
	addIfPresent(foldersMenu, actions.viewInstanceFolder);
	addIfPresent(foldersMenu, actions.viewCentralModsFolder);
	if (actions.viewSelectedInstFolder || actions.viewSelectedMCFolder ||
		actions.viewSelectedModsFolder || actions.viewSelectedConfigFolder) {
		foldersMenu->addSeparator();
		addIfPresent(foldersMenu, actions.viewSelectedInstFolder);
		addIfPresent(foldersMenu, actions.viewSelectedMCFolder);
		addIfPresent(foldersMenu, actions.viewSelectedModsFolder);
		addIfPresent(foldersMenu, actions.viewSelectedConfigFolder);
	}

	// === View =============================================================
	if (actions.toggleCat) {
		auto* viewMenu =
			bar->addMenu(QCoreApplication::translate("MacOSMenuBar", "&View"));
		viewMenu->addAction(actions.toggleCat);
	}

	// === Accounts =========================================================
	auto* accountsMenu = bar->addMenu(
		QCoreApplication::translate("MacOSMenuBar", "&Accounts"));
	addIfPresent(accountsMenu, actions.manageAccounts);
	if (actions.accountSubmenu) {
		// Flatten the dynamic profile list into the menu so users don't have
		// to traverse a nested submenu in the macOS bar.
		const auto subActions = actions.accountSubmenu->actions();
		if (!subActions.isEmpty()) {
			accountsMenu->addSeparator();
			for (auto* a : subActions) {
				if (a) {
					accountsMenu->addAction(a);
				}
			}
		}
	}

	// === Help =============================================================
	auto* helpMenu =
		bar->addMenu(QCoreApplication::translate("MacOSMenuBar", "&Help"));
	addIfPresent(helpMenu, actions.reportBug);
	addIfPresent(helpMenu, actions.discord);
	addIfPresent(helpMenu, actions.reddit);
	if (actions.viewLogs || actions.plugins || actions.checkUpdate) {
		helpMenu->addSeparator();
		addIfPresent(helpMenu, actions.viewLogs);
		addIfPresent(helpMenu, actions.plugins);
		addIfPresent(helpMenu, actions.checkUpdate);
	}
	// AboutRole auto-relocates into the app menu on macOS.
	if (actions.about) {
		helpMenu->addSeparator();
		helpMenu->addAction(actions.about);
	}

	window->setMenuBar(bar);
	return bar;
#endif
}
