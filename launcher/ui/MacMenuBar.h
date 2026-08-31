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

#include <QObject>
#include <QPointer>
#include <QString>

class QEvent;
class QMainWindow;
class QMenu;
class QMenuBar;

/**
 * The screen-top application menu that macOS expects every app to have.
 *
 * No command is defined here. The bar is assembled out of the QActions the
 * main window already owns, found by object name, so a menu entry and its
 * toolbar counterpart are one and the same object: same label, same icon,
 * same shortcut, same enabled state, same slot, and the same wording after
 * the interface language changes.
 *
 * Once attached the bar looks after itself. The profile list mirrors the
 * window's live account menu, a language switch relabels the menu titles,
 * and toggling the setting named by settingKey() attaches or detaches the
 * whole thing without a restart.
 *
 * Off macOS the controller is still constructed and still compiles -- it
 * just never attaches anything. Keeping the code out of a preprocessor
 * guard means the rest of the fleet keeps compile-checking it.
 */
class MacMenuBar : public QObject
{
	Q_OBJECT

  public:
	/// Settings key that decides whether the bar is attached.
	static QString settingKey();

	/// The key this setting shipped under; still honoured when reading.
	static QString legacySettingKey();

	/// Whether this host puts application menus at the top of the screen.
	static bool platformSupported();

	/**
	 * Hand @p window a native menu bar and keep it correct for as long as
	 * the window lives.
	 *
	 * Calling this twice is harmless: the second call returns the
	 * controller the first one created. Only a null @p window yields
	 * nullptr. A controller coming back does not mean a bar is attached --
	 * that is up to the platform and the user's setting.
	 */
	static MacMenuBar* attachTo(QMainWindow* window);

  protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

  private:
	explicit MacMenuBar(QMainWindow* window);

	/// Attach, detach or rebuild so reality matches the current setting.
	void reconcile();
	void attach();
	void detach();

	/// Add the Accounts menu and start tracking the window's profile menu.
	void attachAccounts(QMenuBar* bar);

	/// Add Minimize/Zoom, which macOS users look for and Qt does not add.
	void attachWindowMenu(QMenuBar* bar);

	/// Re-copy the window's profile menu into our Accounts menu.
	void syncAccounts();

	/* Both of these land in the middle of somebody else's work -- a
	 * language switch, or repopulateAccountsMenu() adding one action at a
	 * time -- so the response is deferred and collapsed into a single pass
	 * once the dust has settled. */
	void queueReconcile();
	void queueAccountSync();

	QMainWindow* m_window = nullptr;
	QPointer<QMenuBar> m_bar;
	QPointer<QMenu> m_accounts;      // ours: the copy shown in the bar
	QPointer<QMenu> m_accountSource; // the window's: the original
	bool m_reconcileQueued = false;
	bool m_accountSyncQueued = false;
};
