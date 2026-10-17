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
    // Discover's provider ("modrinth", "curseforge", ...), and those it has
    // searched this session: kept here, like libraryView, so they survive
    // the page being rebuilt. Each provider's model keeps its own results.
    property string discoverProvider: "modrinth"
    property var discoverSearched: []

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

    // Discover's modpack page; Add Content for an instance, and a project
    // from it: see Navigator.qml.
    function openModpack(source, pack) {
        return navigator.openModpack(source, pack)
    }
    function openAddContent(id, kind) {
        return navigator.openAddContent(id, kind)
    }
    function openContentProject(owner, row, project) {
        return navigator.openContentProject(owner, row, project)
    }

    /* Modpack installs started from Discover, by "provider/projectId": the
     * TaskWatcher, which C++ owns, so an install goes on after its page
     * closes and the page finds it again when reopened. A plain object does
     * not notify, so `modpackInstallsRevision` is bumped on every change and
     * read by whatever looks an install up. */
    property var modpackInstalls: ({})
    property int modpackInstallsRevision: 0
    function modpackInstall(key) {
        root.modpackInstallsRevision
        return root.modpackInstalls[key] || null
    }
    // Says how it went when it ends, with a way to the new instance, unless
    // the pack's own page is on screen to say so itself.
    function trackModpackInstall(key, watcher, name) {
        if (!watcher)
            return
        root.modpackInstalls[key] = watcher
        root.modpackInstallsRevision++
        watcher.finished.connect(function (ok) {
            root.modpackInstallsRevision++
            var page = root.pageStack.currentItem
            if (page && page.installKey === key)
                return
            if (ok)
                root.notify(i18n("“%1” is installed.", name), i18nc("@action:button", "Open"),
                            () => root.openInstance(watcher.instanceId))
            else
                root.notify(i18n("Could not install “%1”: %2", name, watcher.error))
        })
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
