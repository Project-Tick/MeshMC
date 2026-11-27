/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

class QEvent;
class QMainWindow;
class QMenu;
class QMenuBar;

/// The native main-window menu, laid out like the final Designer-based menu.
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

    void reconcile();
    void attach();
    QMenu* addMenu(const char* title);
    void attachFolders();
    void attachAccounts();

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
