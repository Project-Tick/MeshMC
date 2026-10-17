// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * What the window's page stack shows: one section's page (Library,
 * Discover, Accounts, Settings, About), and an instance's page pushed over
 * the library. The state it moves -- `section`, `openedInstanceId`,
 * `instancePage` -- lives on Main.qml, which the drawer and pages read.
 */
QtObject {
    id: navigator

    required property var app

    readonly property Component libraryPage: Component {
        LibraryPage { app: navigator.app }
    }
    readonly property Component instancePage: Component {
        InstancePage { app: navigator.app }
    }
    readonly property Component aboutPage: Component {
        FormCard.AboutPage {}
    }
    // The sections whose pages are still to be ported: a placeholder each.
    readonly property Component placeholderPage: Component {
        PlaceholderPage {}
    }
    readonly property var placeholders: ({
        discover: {
            title: i18nc("@title", "Discover"),
            iconName: "system-search",
            text: i18n("Modpacks from Modrinth, CurseForge and more will be here soon")
        },
        accounts: {
            title: i18nc("@title", "Accounts"),
            iconName: "user-identity",
            text: i18n("Your Microsoft and offline accounts will be here soon")
        },
        settings: {
            title: i18nc("@title", "Settings"),
            iconName: "configure",
            text: i18n("The launcher's settings will be here soon")
        }
    })

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
        if (id !== "library" && id !== "about" && !navigator.placeholders[id])
            return false
        if (app.section === id && app.pageStack.depth > 0) {
            if (app.pageStack.depth > 1)
                app.pageStack.pop(app.pageStack.get(0))
            return true
        }
        app.section = id
        app.pageStack.clear()
        if (id === "library")
            navigator.push(navigator.libraryPage)
        else if (id === "about")
            navigator.push(navigator.aboutPage)
        else
            navigator.push(navigator.placeholderPage, navigator.placeholders[id])
        return true
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

    /*
     * Cross-page links: any page can send the user somewhere else in the
     * app with one call. Targets are paths, given whole or split in two
     * (navigate("instance", "fabric") is navigate("instance/fabric")):
     *   "library", "discover", "accounts", "settings", "about"
     *   "settings/<section>"        Settings (its sections come later)
     *   "instance/<id>" or "instance/<id>/<tab>"
     *                               an InstancePage tab id ("content",
     *                               "worlds", "screenshots", "log", ...)
     * Instance ids are folder names and never contain "/". Returns false,
     * leaving the current page alone, for a path it does not know or an
     * instance that does not exist.
     */
    function navigate(page, sub) {
        var path = sub === undefined || sub === null || String(sub).length === 0
                ? String(page) : page + "/" + sub
        var parts = path.split("/")
        switch (parts[0]) {
        case "home":
            return navigator.showSection("library")
        case "library":
        case "discover":
        case "accounts":
        case "settings":
        case "about":
            return navigator.showSection(parts[0])
        case "instance":
            if (parts.length < 2 || parts[1].length === 0)
                return false
            return navigator.openInstance(parts[1], parts.length > 2 ? parts[2] : "")
        }
        return false
    }
}
