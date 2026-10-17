// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * What the window's page stack shows: one section's page (Home, Library,
 * Discover, Accounts, About), with what was opened from it pushed on top --
 * an instance's page over the library, then that instance's Add Content and
 * a project from it; a modpack's page over Discover; an account's skin and
 * cape over Accounts. Settings open in a window of their own
 * (SettingsView.qml). The state
 * it moves -- `section`, `openedInstanceId`, `instancePage` -- lives on
 * Main.qml, which the drawer and pages read.
 */
QtObject {
    id: navigator

    required property var app

    readonly property Component homePage: Component {
        HomePage { app: navigator.app }
    }
    readonly property Component libraryPage: Component {
        LibraryPage { app: navigator.app }
    }
    readonly property Component instancePage: Component {
        InstancePage { app: navigator.app }
    }
    readonly property Component aboutPage: Component {
        AboutMeshMCPage { app: navigator.app }
    }
    readonly property Component discoverPage: Component {
        DiscoverPage { app: navigator.app }
    }
    readonly property Component modpackPage: Component {
        ModpackPage { app: navigator.app }
    }
    readonly property Component addContentPage: Component {
        AddContentPage { app: navigator.app }
    }
    readonly property Component contentProjectPage: Component {
        ContentProjectPage { app: navigator.app }
    }
    readonly property Component accountsPage: Component {
        AccountsPage { app: navigator.app }
    }
    readonly property Component accountSkinPage: Component {
        AccountSkinPage { app: navigator.app }
    }
    readonly property Component newInstancePage: Component {
        NewInstancePage { app: navigator.app }
    }
    readonly property Component importInstancePage: Component {
        ImportInstancePage { app: navigator.app }
    }

    /* Pages are made here, with the page stack as their parent, rather than
     * handed to it as components: PageRow parents what it makes to a plain
     * QtObject first, which Qt Quick reports as an item "not placed in the
     * graphics scene". So they are also deleted here once the stack lets
     * them go. */
    function push(component, properties) {
        var page = component.createObject(navigator.app.pageStack, properties || {})
        navigator.app.pageStack.push(page)
        return page
    }
    readonly property Connections removedPages: Connections {
        target: navigator.app.pageStack
        function onPageRemoved(page) {
            page.destroy()
        }
    }

    // Shows the page of section @p id, or the current page again when it is
    // already showing; an instance page open over the library closes.
    function showSection(id) {
        var app = navigator.app
        // Settings are a window of their own (SettingsView.qml), over
        // whatever the main window shows.
        if (id === "settings")
            return app.openSettings("")
        if (id !== "home" && id !== "library" && id !== "about" && id !== "discover" && id !== "accounts")
            return false
        if (app.section === id && app.pageStack.depth > 0) {
            if (app.pageStack.depth > 1)
                app.pageStack.pop(app.pageStack.get(0))
            return true
        }
        app.section = id
        app.pageStack.clear()
        if (id === "home")
            navigator.push(navigator.homePage)
        else if (id === "library")
            navigator.push(navigator.libraryPage)
        else if (id === "discover")
            navigator.push(navigator.discoverPage)
        else if (id === "about")
            navigator.push(navigator.aboutPage)
        else
            navigator.push(navigator.accountsPage)
        return true
    }

    // The skin and cape of account @p row (AccountList's row; -1 is the
    // review demo, see AccountsController::skinDemoRequested()) over the
    // Accounts page.
    function openAccountSkin(row, accountId, name) {
        navigator.showSection("accounts")
        return navigator.push(navigator.accountSkinPage, { row: row, accountId: accountId || "", accountName: name || "" })
    }

    // New Instance over the library, or the one already showing.
    function openNewInstance() {
        var top = navigator.app.pageStack.currentItem
        if (top && top.objectName === "newInstancePage")
            return top
        navigator.showSection("library")
        return navigator.push(navigator.newInstancePage)
    }
    // Import over the library, with @p source (a path, a file:// URL or a
    // link) filled in when given. The Import page already showing takes the
    // new source in place, unless it is still importing the last one.
    function openImport(source) {
        var app = navigator.app
        var top = app.pageStack.currentItem
        if (top && top.objectName === "importInstancePage") {
            if (top.importing)
                app.notify(i18n("Another instance is still being imported. Try again once it is in your library."))
            else if (source)
                top.setSource(source)
            return top
        }
        navigator.showSection("library")
        return navigator.push(navigator.importInstancePage, { source: source || "" })
    }

    // The page of instance @p id over the library, on @p tab when given (a
    // tab it does not have shows its first one). False for an unknown id.
    function openInstance(id, tab) {
        var app = navigator.app
        var details = app.shell && typeof app.shell.instanceDetails === "function"
                ? app.shell.instanceDetails(id) : null
        if (!details)
            return false
        if (app.instancePage && app.openedInstanceId === id) {
            // Back to the instance from whatever was opened over it.
            if (app.pageStack.currentItem !== app.instancePage)
                app.pageStack.pop(app.instancePage)
            if (tab)
                app.instancePage.tab = tab
            return true
        }
        navigator.showSection("library")
        app.openedInstanceId = id
        app.instancePage = navigator.push(navigator.instancePage, {
            details: details,
            instanceId: id,
            tab: tab || ""
        })
        return true
    }

    // Discover's modpack @p pack (a result row's roles) over Discover, from
    // @p source: {provider, providerName, model, installer} (DiscoverPage).
    function openModpack(source, pack) {
        if (!source || !source.model || !pack || !pack.projectId)
            return null
        navigator.showSection("discover")
        return navigator.push(navigator.modpackPage, { source: source, pack: pack })
    }

    // Add Content over instance @p id's page, on @p kind ("mods",
    // "resourcepacks", "shaderpacks", "datapacks") when given. False for an
    // unknown instance, or one with nothing to browse for.
    function openAddContent(id, kind) {
        var app = navigator.app
        if (!navigator.openInstance(id, "content"))
            return false
        var details = app.instancePage ? app.instancePage.details : null
        if (!details || details.isLegacy)
            return false
        navigator.push(navigator.addContentPage, {
            details: details,
            installer: app.instancePage.contentInstaller,
            initialKind: kind || ""
        })
        return true
    }

    // Project @p project of Add Content's results, row @p row, over the
    // Add Content page @p owner.
    function openContentProject(owner, row, project) {
        var app = navigator.app
        if (!owner || app.pageStack.currentItem !== owner)
            return null
        return navigator.push(navigator.contentProjectPage, {
            owner: owner,
            row: row,
            project: project
        })
    }

    /*
     * Cross-page links: any page can send the user somewhere else in the
     * app with one call. Targets are paths, given whole or split in two
     * (navigate("instance", "fabric") is navigate("instance/fabric")):
     *   "home", "library", "discover", "accounts", "settings", "about"
     *   "settings/<page>"           the settings window on that page
 *                               (SettingsView.qml lists the ids)
     *   "discover/<provider>"       Discover on "modrinth", "curseforge",
     *                               "ftb", "atlauncher" or "technic"
     *   "instance/<id>" or "instance/<id>/<tab>"
     *                               an InstancePage tab id ("content",
     *                               "worlds", "screenshots", "log", ...)
     *   "instance/<id>/add" or "instance/<id>/add/<kind>"
     *                               Add Content for it, on a kind
     *                               ("mods", "resourcepacks", ...)
     * Instance ids are folder names and never contain "/". Returns false,
     * leaving the current page alone, for a path it does not know or an
     * instance that does not exist.
     */
    function navigate(page, sub) {
        var path = sub === undefined || sub === null || String(sub).length === 0
                ? String(page) : page + "/" + sub
        var parts = path.split("/")
        switch (parts[0]) {
        case "discover":
            if (parts.length > 1 && parts[1].length > 0)
                navigator.app.discoverProvider = parts[1]
            return navigator.showSection("discover")
        case "settings":
            return navigator.app.openSettings(parts.length > 1 ? parts[1] : "")
        case "home":
        case "library":
        case "accounts":
        case "about":
            return navigator.showSection(parts[0])
        case "instance":
            if (parts.length < 2 || parts[1].length === 0)
                return false
            if (parts.length > 2 && parts[2] === "add")
                return navigator.openAddContent(parts[1], parts.length > 3 ? parts[3] : "")
            return navigator.openInstance(parts[1], parts.length > 2 ? parts[2] : "")
        }
        return false
    }
}
