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
 *   size=WxH             the window's size
 *   page=<path>, nav=<path>
 *                        Main.qml's navigate() ("discover", "instance/fabric/log")
 *   instance=<id>        that instance's page; tab=<id> then picks its tab,
 *                        logfile=<name> shows one of its log files,
 *                        launching[=<status>:<0..1>] its footer mid-launch
 *   view=grid|list       the library's view
 *   sidebar=collapsed|expanded
 *                        folds the wide window's sidebar
 *   drawer=open          opens the narrow window's modal drawer
 *   menu=<id>            the library's context menu for that instance
 *   theme=light|dark     a colour scheme (QmlShell::debugColorScheme())
 *   newinstance          the New Instance dialog
 *   ask=pickAccount|busySkip
 *                        a question only a real launch asks
 *                        (QmlShell::debugShowRequest())
 *   crashed=<id>         marks the instance as crashed last time
 *   click=x,y[,right[,delayMs]]
 *                        a real click at a window point (QmlShell::debugClick())
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

    function apply(text) {
        var app = route.app
        var steps = text.split(";")
        for (var i = 0; i < steps.length; ++i) {
            var eq = steps[i].indexOf("=")
            var key = (eq < 0 ? steps[i] : steps[i].slice(0, eq)).trim()
            var value = eq < 0 ? "" : steps[i].slice(eq + 1).trim()
            if (key === "size") {
                var wh = value.split("x")
                app.width = parseInt(wh[0])
                app.height = parseInt(wh[1])
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
            else if (key === "ask")
                app.call("debugShowRequest", value)
            else if (key === "crashed")
                app.call("debugMarkInstanceCrashed", value)
            else if (key === "click")
                app.call("debugClick", value)
        }
    }
}
