/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "MacMenuBar.h"

#include <QAction>
#include <QCoreApplication>
#include <QEvent>
#include <QDir>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QTimer>
#include <QVariant>

#include "Application.h"
#include "BuildConfig.h"
#include "DesktopServices.h"
#include "settings/Setting.h"
#include "settings/SettingsObject.h"

namespace
{
    /// Translation context for the menu titles coined in this file.
    const char* const kContext = "MainWindow";

    /// Name of the profile menu the main window keeps for its toolbar.
    const char* const kAccountMenuName = "accountMenu";

    // Each entry names a main window action. Missing optional actions are skipped.
    const char* const kDivider = "|";

    inline bool isDivider(const char* entry)
    {
        return entry[0] == '|';
    }

    struct MenuSpec {
        const char* title;          // untranslated, context kContext
        const char* const* entries; // nullptr-terminated
    };

    // Keep the order of the last MainWindow.ui from old-develop.
    const char* const kFileEntries[] = {
        "actionAddInstance",
        kDivider,
        "actionLaunchInstance", "actionLaunchInstanceOffline",
        kDivider,
        "actionEditInstance", "actionChangeInstGroup", "actionViewSelectedInstFolder",
        "actionExportInstance", "actionCopyInstance", "actionDeleteInstance", "actionCreateShortcut",
        "actionSettings",
        nullptr,
    };

    const char* const kEditEntries[] = {
        "actionRenameInstance", "actionChangeInstIcon",
        nullptr,
    };

    const char* const kInstanceEntries[] = {
        "actionInstanceSettings", "actionEditInstNotes",
        kDivider,
        "actionMods", "actionWorlds", "actionScreenshots",
        kDivider,
        "actionViewSelectedMCFolder", "actionViewSelectedModsFolder", "actionConfig_Folder",
        nullptr,
    };

    const char* const kViewEntries[] = {
        "actionCAT",
        nullptr,
    };

    const char* const kHelpEntries[] = {
        "actionReportBug", "actionDISCORD", "actionREDDIT", "actionMoreNews",
        kDivider,
        "actionCheckUpdate", "actionAbout",
        nullptr,
    };

    const MenuSpec kLeadingMenus[] = {
        {QT_TRANSLATE_NOOP("MainWindow", "&File"), kFileEntries},
        {QT_TRANSLATE_NOOP("MainWindow", "&Edit"), kEditEntries},
        {QT_TRANSLATE_NOOP("MainWindow", "&Instance"), kInstanceEntries},
        {QT_TRANSLATE_NOOP("MainWindow", "&View"), kViewEntries},
    };

    /**
     * Fill @p menu, turning null entries and separators into rules.
     *
     * A rule is only committed once something follows it, which is what
     * drops rules that would otherwise lead, trail, or double up after the
     * absent actions around them have been skipped.
     *
     * Returns whether anything at all was added.
     */
    bool appendGroup(QMenu* menu, const QList<QAction*>& actions)
    {
        bool dividerPending = false;
        bool anyAdded = false;

        for (QAction* action : actions) {
            if (!action || action->isSeparator()) {
                dividerPending = anyAdded;
                continue;
            }
            if (dividerPending) {
                menu->addSeparator();
                dividerPending = false;
            }
            menu->addAction(action);
            anyAdded = true;
        }

        return anyAdded;
    }

    /// Turn a table of object names into actions; dividers become nulls.
    QList<QAction*> resolveEntries(const QMainWindow* window,
                                   const char* const* entries)
    {
        QList<QAction*> resolved;

        for (; entries && *entries; ++entries) {
            if (isDivider(*entries)) {
                resolved.append(nullptr);
                continue;
            }
            /* Direct children only. Every toolbar action hangs straight off
             * the main window, and insisting on that keeps a same-named
             * action buried in some child widget from being picked up. */
            if (auto* action = window->findChild<QAction*>(
                    QString::fromLatin1(*entries), Qt::FindDirectChildrenOnly)) {
                resolved.append(action);
            }
        }

        return resolved;
    }

    /// Read the toggle, defaulting to on while settings are still coming up.
    bool settingEnabled()
    {
        if (!APPLICATION || !APPLICATION->settings()) {
            return true;
        }
        return APPLICATION->settings()->get(MacMenuBar::settingKey()).toBool();
    }
} // namespace

QString MacMenuBar::settingKey()
{
    return QStringLiteral("MacNativeMenuBar");
}

QString MacMenuBar::legacySettingKey()
{
    return QStringLiteral("UseMacNativeMenuBar");
}

bool MacMenuBar::platformSupported()
{
    #ifdef Q_OS_MAC
    return true;
#else
    return false;
#endif
}

MacMenuBar* MacMenuBar::attachTo(QMainWindow* window)
{
    if (!window) {
        return nullptr;
    }

    auto* existing =
        window->findChild<MacMenuBar*>(QString(), Qt::FindDirectChildrenOnly);
    if (existing) {
        return existing;
    }

    return new MacMenuBar(window);
}

MacMenuBar::MacMenuBar(QMainWindow* window) : QObject(window), m_window(window)
{
    if (!platformSupported()) {
        /* None of this means anything without a screen-top menu bar. The
         * object is still handed back so callers need no platform test of
         * their own, and the code below is still compiled everywhere --
         * it simply never runs here. */
        return;
    }

    m_window->installEventFilter(this);

    if (APPLICATION && APPLICATION->settings()) {
        auto* settings = APPLICATION->settings().get();
        const auto reconcileSetting = [this](const Setting& setting) {
            if (setting.id() == settingKey()) {
                queueReconcile();
            }
        };
        connect(settings, &SettingsObject::SettingChanged, this, reconcileSetting);
        connect(settings, &SettingsObject::settingReset, this, reconcileSetting);
    }

    reconcile();
}

void MacMenuBar::reconcile()
{
    const bool enabled = settingEnabled();
    if (!m_bar) {
        if (!enabled)
            return;
        attach();
    }

    for (auto menu : m_bar->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly)) {
        menu->menuAction()->setVisible(enabled);
        const auto title = menu->property("sourceTitle").toByteArray();
        menu->setTitle(QCoreApplication::translate(kContext, title.constData()));
        for (auto action : menu->actions()) {
            const auto text = action->property("sourceText").toByteArray();
            if (!text.isEmpty())
                action->setText(QCoreApplication::translate(kContext, text.constData()));
        }
    }
}

void MacMenuBar::attach()
{
    m_bar = new QMenuBar(m_window);
    m_bar->setObjectName(QStringLiteral("macMenuBar"));
    m_bar->setNativeMenuBar(true);
    for (const MenuSpec& spec : kLeadingMenus) {
        auto* menu = addMenu(spec.title);
        appendGroup(menu, resolveEntries(m_window, spec.entries));
    }
    attachFolders();
    attachAccounts();
    appendGroup(addMenu(QT_TRANSLATE_NOOP("MainWindow", "&Help")), resolveEntries(m_window, kHelpEntries));
    m_window->setMenuBar(m_bar);
}

QMenu* MacMenuBar::addMenu(const char* title)
{
    auto* menu = new QMenu(m_bar);
    menu->setProperty("sourceTitle", QByteArray(title));
    menu->setToolTipsVisible(true);
    m_bar->addMenu(menu);
    return menu;
}

void MacMenuBar::attachFolders()
{
    auto* menu = addMenu(QT_TRANSLATE_NOOP("MainWindow", "F&olders"));
    const auto addFolder = [menu](const char* title, const char* path, bool fromSettings = false) {
        auto* action = menu->addAction(QString());
        action->setMenuRole(QAction::NoRole);
        action->setProperty("sourceText", QByteArray(title));
        connect(action, &QAction::triggered, menu, [path, fromSettings] {
            const QString folder = fromSettings ? APPLICATION->settings()->get(QString::fromLatin1(path)).toString()
                                                : QString::fromLatin1(path);
            if (!folder.isEmpty() && QDir().mkpath(folder))
                DesktopServices::openDirectory(folder);
        });
    };

    addFolder(QT_TRANSLATE_NOOP("MainWindow", "Launcher Root"), ".");
    menu->addSeparator();
    const char* const sharedFolders[] = {"actionViewInstanceFolder", "actionViewCentralModsFolder", nullptr};
    appendGroup(menu, resolveEntries(m_window, sharedFolders));
    addFolder(QT_TRANSLATE_NOOP("MainWindow", "Skins"), "SkinsDir", true);
    addFolder(QT_TRANSLATE_NOOP("MainWindow", "Java"), "JavaDir", true);
    menu->addSeparator();
    addFolder(QT_TRANSLATE_NOOP("MainWindow", "Icon Theme"), "iconthemes");
    addFolder(QT_TRANSLATE_NOOP("MainWindow", "Widget Themes"), "themes");
    addFolder(QT_TRANSLATE_NOOP("MainWindow", "Instance Icons"), "IconsDir", true);
    menu->addSeparator();
    auto* logs = menu->addAction(QString());
    logs->setMenuRole(QAction::NoRole);
    logs->setProperty("sourceText", QByteArray(QT_TRANSLATE_NOOP("MainWindow", "Logs")));
    connect(logs, &QAction::triggered, menu, [] {
        DesktopServices::openFile(BuildConfig.LAUNCHER_NAME + "-0.log");
    });
}

void MacMenuBar::attachAccounts()
{
    m_accountSource = m_window->findChild<QMenu*>(
        QString::fromLatin1(kAccountMenuName), Qt::FindDirectChildrenOnly);
    if (!m_accountSource) {
        return;
    }

    /* The window rebuilds that menu from scratch whenever the account list
     * moves, so watch it rather than reading it once. Copying is safer than
     * sharing the menu outright: the toolbar button owns the original as a
     * popup, and the native bar wants a menu of its own. */
    m_accountSource->installEventFilter(this);

    m_accounts = addMenu(QT_TRANSLATE_NOOP("MainWindow", "&Accounts"));
    syncAccounts();
}

void MacMenuBar::syncAccounts()
{
    if (!m_accounts) {
        return;
    }

    /* Only the rules were created here, so clearing takes them and leaves
     * the window's account actions -- which are merely on loan -- alone. */
    m_accounts->clear();

    if (!m_accountSource) {
        return;
    }
    appendGroup(m_accounts.data(), m_accountSource->actions());
}

void MacMenuBar::queueReconcile()
{
    if (m_reconcileQueued) {
        return;
    }
    m_reconcileQueued = true;

    QTimer::singleShot(0, this, [this] {
        m_reconcileQueued = false;
        reconcile();
    });
}

void MacMenuBar::queueAccountSync()
{
    if (m_accountSyncQueued) {
        return;
    }
    m_accountSyncQueued = true;

    QTimer::singleShot(0, this, [this] {
        m_accountSyncQueued = false;
        syncAccounts();
    });
}

bool MacMenuBar::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_window) {
        if (event->type() == QEvent::LanguageChange) {
            queueReconcile();
        }
    } else if (watched == m_accountSource.data()) {
        switch (event->type()) {
            case QEvent::ActionAdded:
            case QEvent::ActionChanged:
            case QEvent::ActionRemoved:
                queueAccountSync();
                break;
            default:
                break;
        }
    }

    return QObject::eventFilter(watched, event);
}
