// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * The launcher as a stock KDE application: a Kirigami window whose global
 * drawer is the sidebar (Library, Discover, Accounts, Settings, About and
 * the recently played instances) and whose page stack holds one section's
 * page, with an instance's page pushed on top of the library.
 *
 * Everything the pages do goes through the functions below, so the library,
 * an instance page and the drawer answer the same request the same way:
 * navigate() for links across the app, launch() and friends for playing,
 * and `dialogs` for managing an instance. The pages are handed this window
 * as `app` rather than reaching for it by id.
 */
Kirigami.ApplicationWindow {
    id: root

    required property var instanceModel
    required property var selection
    // QmlShell: actions (launchInstance, killInstance, ...), the account
    // summary and the extra models the pages need.
    required property var shell

    readonly property string moduleName: "MeshMC.Kirigami"

    // Handed over by QmlShell::rootProperties(); the KDE interface has no
    // cat, so it is only accepted.
    property bool catAvailable: false

    title: i18nc("@title:window", "MeshMC")
    width: Kirigami.Units.gridUnit * 66
    height: Kirigami.Units.gridUnit * 42
    minimumWidth: Kirigami.Units.gridUnit * 22
    minimumHeight: Kirigami.Units.gridUnit * 20
    // A Kirigami window shows itself as soon as it exists; QmlShell::show()
    // decides when instead, and whether minimised.
    visible: false

    // Below this the sidebar becomes a modal drawer behind the toolbar's
    // menu button, as on a phone; above it, it stays beside the pages.
    readonly property bool wideLayout: width >= Kirigami.Units.gridUnit * 50

    // "library", "discover", "accounts", "settings" or "about".
    property string section: "library"
    // The instance whose page is open over the library, or "".
    property string openedInstanceId: ""
    // That page, while it is on the stack.
    property var instancePage: null
    // The library's grid or list, kept here so it survives the library page
    // being rebuilt; session-only, as it was before.
    property string libraryView: "grid"

    // One section page at a time, with an instance page pushed over the
    // library: never two columns side by side, which is for master-detail
    // views this app does not have.
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.ToolBar
    pageStack.globalToolBar.showNavigationButtons: Kirigami.ApplicationHeaderStyle.ShowBackButton

    globalDrawer: AppDrawer {
        id: drawer
        app: root
    }

    // Instance management for every page: rename, icon, group, duplicate,
    // export, shortcut and delete.
    readonly property alias dialogs: instanceDialogs
    InstanceDialogs {
        id: instanceDialogs
        app: root
    }

    // The core's questions and its "please wait" (QmlUiHost).
    UiRequests {
        host: root.shell && root.shell.uiHost ? root.shell.uiHost : null
    }

    // Which page the stack shows: see Navigator.qml.
    Navigator {
        id: navigator
        app: root
    }
    function showSection(id) {
        return navigator.showSection(id)
    }
    function openInstance(id, tab) {
        return navigator.openInstance(id, tab)
    }
    // Cross-page links ("settings", "instance/fabric/log", ...): see
    // Navigator.navigate(). False for a path it does not know.
    function navigate(page, sub) {
        return navigator.navigate(page, sub)
    }
    // The instance page closed, by Back, Escape or a section change.
    function instancePageClosed(page) {
        if (root.instancePage !== page)
            return
        root.instancePage = null
        root.openedInstanceId = ""
    }

    function call(name, arg) {
        if (root.shell && typeof root.shell[name] === "function")
            arg === undefined ? root.shell[name]() : root.shell[name](arg)
    }

    // A passing message at the bottom of the window, optionally with one
    // action -- the way every Kirigami application reports what happened.
    function notify(message, actionText, callback) {
        root.showPassiveNotification(message, "short", actionText || "", callback || null)
    }

    // Play, everywhere. With no account at all the launch flow could only
    // ask whether to open the Accounts page; go there straight away and say
    // what is missing instead.
    function launch(id) {
        if (root.sendToAccountsIfNone())
            return
        root.call("launchInstance", id)
    }
    function stop(id) {
        root.call("killInstance", id)
    }
    // "offline" or "demo" (QmlShell::launchInstanceMode()). The demo needs
    // no account; offline does.
    function launchMode(id, mode) {
        if (mode !== "demo" && root.sendToAccountsIfNone())
            return
        if (root.shell && typeof root.shell.launchInstanceMode === "function")
            root.shell.launchInstanceMode(id, mode)
    }
    // What the Play options offer for instance @p id (QmlShell::launchOptions()).
    function launchOptions(id) {
        return root.shell && typeof root.shell.launchOptions === "function"
                ? root.shell.launchOptions(id) : ({})
    }
    function sendToAccountsIfNone() {
        if (!root.shell || root.shell.accountCount !== 0)
            return false
        root.navigate("accounts")
        root.notify(i18n("Sign in with the Microsoft account that owns Minecraft to play."))
        return true
    }

    // Every group in use, with "" (no group) first.
    function instanceGroups() {
        var groups = root.shell && root.shell.groups ? root.shell.groups : []
        return [""].concat(groups.filter(g => g.length > 0))
    }

    // "New instance" and "Import": the dialog moves over in a later phase.
    function openNewInstance(mode, source) {
        root.dialogs.newInstance(mode === "import", source || "")
    }
    function openImport(source) {
        root.openNewInstance("import", source)
    }

    Shortcut {
        sequences: [StandardKey.New]
        onActivated: root.openNewInstance()
    }
    // Escape leaves an instance page for the library, as Back does. A menu
    // or dialog that is open takes the key first.
    Shortcut {
        sequences: [StandardKey.Cancel]
        enabled: root.pageStack.depth > 1
        onActivated: root.pageStack.pop()
    }

    Connections {
        target: root.shell
        ignoreUnknownSignals: true
        // The game's console, asked for by the launch flow (ShowConsole, or
        // a crash): the instance page, on its Log tab.
        function onOpenInstanceLog(id) {
            root.openInstance(id, "log")
        }
        function onImportRequested(source) {
            root.openImport(source)
        }
        // From C++ (QmlShell::requestNavigate()): a launch that needs an
        // account opens the Accounts page, for one.
        function onNavigateRequested(target) {
            root.navigate(target)
        }
    }

    // The single-row model the instance page shows its header from.
    Binding {
        target: root.shell && root.shell.instancePageModel ? root.shell.instancePageModel : null
        property: "instanceId"
        value: root.openedInstanceId.length > 0 ? root.openedInstanceId : "/"
        when: !!root.shell && !!root.shell.instancePageModel
    }
    // The library's selection follows the open instance (plugins read it).
    onOpenedInstanceIdChanged: {
        if (!root.selection)
            return
        if (root.openedInstanceId.length > 0 && typeof root.selection.selectOnly === "function")
            root.selection.selectOnly(root.openedInstanceId)
        else if (typeof root.selection.clear === "function")
            root.selection.clear()
    }

    Component.onCompleted: {
        root.showSection("library")
        drawer.restoreState()
        if (root.devRoute.length > 0)
            Qt.callLater(routeSteps.apply, root.devRoute)
    }

    // Startup route for review snapshots (MESHMC_QML_ROUTE): see DevRoute.qml.
    property string devRoute: ""
    DevRoute {
        id: routeSteps
        app: root
        drawer: drawer
    }
}
