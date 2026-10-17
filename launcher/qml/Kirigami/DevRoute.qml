// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * The startup route for review snapshots (MESHMC_QML_ROUTE): ";"-separated
 * steps such as "size=800x700;page=library;view=list" or
 * "instance=fabric;tab=log", run in order once the window exists.
 *
 *   size=WxH             the window's size -- the settings window's while
 *                        it is open
 *   page=<path>, nav=<path>
 *                        Main.qml's navigate() ("discover", "instance/fabric/log",
 *                        "settings/java" opens the settings window)
 *   instance=<id>        that instance's page; tab=<id> then picks its tab
 *                        (content, worlds, screenshots, log, settings, and
 *                        under More: managedpack, version, datapacks,
 *                        servers, backups, gameoptions, notes, plugins),
 *                        logfile=<name> shows one of its log files,
 *                        launching[=<status>:<0..1>] its footer mid-launch
 *   view=grid|list       the library's view
 *   sidebar=collapsed|expanded
 *                        folds the wide window's sidebar
 *   drawer=open          opens the narrow window's modal drawer
 *   menu=<id>            the library's context menu for that instance
 *   theme=light|dark     the system appearance the colour scheme follows
 *                        (QmlShell::debugColorScheme(), ColorSchemes)
 *   newinstance[=import] the New Instance page (or Import); version=<n>
 *                        then picks version n, loader=<id> a mod loader
 *                        ("" vanilla, fabric, quilt, forge, neoforge) and
 *                        install presses Create (a real install)
 *   import=<path|url>    the Import page with that file or link, as a drop
 *                        on the window or `meshmc --import` opens it
 *   export=<id>[:<format>]
 *                        the Export dialog of that instance, on "zip",
 *                        "modrinth" or "curseforge"
 *   onboarding[=<step>]  the first-run flow, with every step, on "welcome",
 *                        "account", "java" or "done"
 *   ask=pickAccount|busySkip|choose
 *                        a question only a real launch asks
 *                        (QmlShell::debugShowRequest())
 *   crashed=<id>         marks the instance as crashed last time
 *   click=x,y[,right[,delayMs]]
 *                        a real click at a window point (QmlShell::debugClick())
 *
 * Discover and Add Content (the page on top of the stack):
 *   provider=<id>        Discover's provider, or Add Content's site
 *   kind=<id>            Add Content's kind ("mods", "resourcepacks", ...)
 *   search=<words>       the page's search
 *   filters              its Filters dialog
 *   detail=<n>           opens result n once the search has answered
 *   version=<n>          the project page's version n, once listed
 *   install              presses Install once the page can (modpack or
 *                        project page) -- a real install: use it only with a
 *                        data folder of its own
 *   gallery=<n>          the modpack's picture n, full size
 *   scroll=<where>       the page scrolled to "description", "bottom" or a
 *                        pixel offset, once that part has loaded
 *   updates, export      the Content tab's Check for Updates and Export
 *                        List dialogs (after instance=<id>)
 *   review=<name>        the open instance tab's own state, once built:
 *                        version: minecraftversion, loader, newcomponent;
 *                        worlds: rename, delete, running; datapacks: add;
 *                        servers: add, edit; backups: create;
 *                        screenshots: select, rename, delete
 *   wait=<ms>            pauses the steps after it
 *
 * The steps from detail= on wait for the network: each runs once the page
 * can do it (asked again every few frames, given up after a while), and
 * the steps after it run only then.
 */
QtObject {
    id: route

    required property var app
    required property var drawer

    // A size step resizes the window asynchronously, and the drawer closes
    // itself when that makes it modal: open it once the size has settled.
    readonly property Timer drawerDelay: Timer {
        id: drawerDelay
        interval: Kirigami.Units.veryLongDuration
        onTriggered: route.drawer.open()
    }

    // The steps still to run, and the one being waited on: a function that
    // returns true once it has done its step.
    property var pending: []
    property var waitingOn: null
    property int waitedMs: 0
    readonly property int pollMs: 200
    readonly property int giveUpMs: 30000
    readonly property Timer poll: Timer {
        interval: route.pollMs
        repeat: true
        onTriggered: {
            route.waitedMs += interval
            var done = false
            try {
                done = route.waitingOn()
            } catch (e) {
                done = false
            }
            if (!done && route.waitedMs < route.giveUpMs)
                return
            if (!done)
                console.warn("DevRoute: gave up waiting on a step")
            stop()
            route.waitingOn = null
            route.runPending()
        }
    }
    // Runs @p fn now if it can be done now, else every pollMs until it is.
    function waitFor(fn) {
        try {
            if (fn())
                return
        } catch (e) {
        }
        route.waitingOn = fn
        route.waitedMs = 0
        route.poll.restart()
    }
    // The settings window while it is open, else the main one: what size=
    // and the page steps act on.
    function front() {
        var settings = route.app.settingsView ? route.app.settingsView.settingsWindow : null
        return settings && settings.pageStack ? settings : route.app
    }
    // The page on top of the front window's stack, if it has @p method.
    function top(method) {
        var page = route.front().pageStack.currentItem
        return page && typeof page[method] === "function" ? page : null
    }

    function apply(text) {
        route.pending = text.split(";")
        route.runPending()
    }
    function runPending() {
        while (route.pending.length > 0 && !route.waitingOn) {
            var step = route.pending[0]
            route.pending = route.pending.slice(1)
            route.applyStep(step)
        }
    }
    function applyStep(step) {
        var app = route.app
        {
            var eq = step.indexOf("=")
            var key = (eq < 0 ? step : step.slice(0, eq)).trim()
            var value = eq < 0 ? "" : step.slice(eq + 1).trim()
            if (key === "size") {
                var wh = value.split("x")
                route.front().width = parseInt(wh[0])
                route.front().height = parseInt(wh[1])
            } else if (key === "page" || key === "nav")
                app.navigate(value)
            else if (key === "instance")
                app.openInstance(value)
            else if (key === "tab" && app.instancePage)
                app.instancePage.tab = value
            else if (key === "logfile" && app.instancePage)
                app.instancePage.showLogFile(value)
            else if (key === "launching" && app.instancePage) {
                var launch = value.length > 0 ? value.split(":") : [i18n("Downloading assets"), "0.42"]
                app.instancePage.previewLaunch = {
                    status: launch[0],
                    progress: launch.length > 1 ? parseFloat(launch[1]) : -1
                }
            }
            else if (key === "view")
                app.libraryView = value === "list" ? "list" : "grid"
            else if (key === "sidebar")
                route.drawer.collapsed = value === "collapsed"
            else if (key === "drawer")
                drawerDelay.start()
            else if (key === "menu" && app.pageStack.currentItem
                     && typeof app.pageStack.currentItem.openMenuFor === "function")
                Qt.callLater(app.pageStack.currentItem.openMenuFor, value)
            else if (key === "theme")
                app.call("debugColorScheme", value)
            else if (key === "newinstance")
                app.openNewInstance(value)
            else if (key === "import")
                app.openImport(value)
            else if (key === "onboarding")
                app.openOnboarding(true, value)
            else if (key === "loader")
                route.waitFor(() => !!route.top("pickLoader") && route.top("pickLoader").pickLoader(value))
            // With an instance it is the Export dialog; bare, the Content
            // tab's Export List (below).
            else if (key === "export" && value.length > 0) {
                var exportArgs = value.split(":")
                app.dialogs.exportInstance(exportArgs[0], exportArgs[1] || "")
            }
            else if (key === "ask")
                app.call("debugShowRequest", value)
            else if (key === "crashed")
                app.call("debugMarkInstanceCrashed", value)
            else if (key === "click")
                app.call("debugClick", value)
            else if (key === "provider") {
                if (route.top("setProvider"))
                    route.top("setProvider").setProvider(value)
                else
                    app.discoverProvider = value
            } else if (key === "kind" && route.top("showKind"))
                route.top("showKind").showKind(value)
            else if (key === "search" && route.top("setSearch"))
                route.top("setSearch").setSearch(value)
            else if (key === "filters")
                route.waitFor(() => !!route.top("openFilters") && route.top("openFilters").openFilters())
            else if (key === "detail")
                route.waitFor(() => !!route.top("openResult") && route.top("openResult").openResult(parseInt(value)))
            else if (key === "version")
                route.waitFor(() => !!route.top("pickVersion") && route.top("pickVersion").pickVersion(parseInt(value)))
            else if (key === "install")
                route.waitFor(() => !!route.top("installWhenReady") && route.top("installWhenReady").installWhenReady())
            else if (key === "gallery")
                route.waitFor(() => !!route.top("openGallery") && route.top("openGallery").openGallery(parseInt(value)))
            else if (key === "scroll")
                route.waitFor(() => !!route.top("scrollTo") && route.top("scrollTo").scrollTo(value))
            else if (key === "updates" || key === "export")
                route.waitFor(() => !!app.instancePage && app.instancePage.openContentDialog(key))
            else if (key === "review")
                route.waitFor(() => !!app.instancePage && app.instancePage.review(value))
            else if (key === "wait") {
                var until = Date.now() + parseInt(value)
                route.waitFor(() => Date.now() >= until)
            }
        }
    }
}
