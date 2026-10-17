// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * One instance, pushed over the library: its name as the page title, Play
 * (or Stop) and Open Folder in the toolbar with everything else in its
 * overflow menu, and tabs for what is inside it -- Content, Worlds,
 * Screenshots, Log and Settings. While the game starts, the footer shows the
 * launch's progress and Cancel.
 *
 * The page reads its instance from two places: `details` (InstanceDetails,
 * QmlShell.instanceDetails()) for the tabs, and the shell's single-row
 * instancePageModel, which Main.qml points at this instance, for the live
 * name, icon and launch state the library shows too.
 */
Kirigami.Page {
    id: page

    required property var app
    property var details: null
    property string instanceId
    // The open tab's id; empty, or a tab this instance does not have, shows
    // the first one.
    property string tab: ""

    // The instance's row of instancePageModel, while it has one.
    property var row: null
    readonly property bool launching: !!page.row && (page.row.launchStatus || "").length > 0
    readonly property bool running: !!page.row && page.row.isRunning === true
    readonly property bool isLegacy: !!page.details && page.details.isLegacy
    // A launch's progress without a launch, for review snapshots
    // (DevRoute.qml's "launching=<status>:<0..1>"): {status, progress}.
    property var previewLaunch: null

    // An old-format instance can only be upgraded, so it has no Content or
    // Settings.
    readonly property var tabs: {
        var list = []
        if (!page.isLegacy)
            list.push({ id: "content", text: i18nc("@title:tab", "Content") })
        list.push({ id: "worlds", text: i18nc("@title:tab", "Worlds") })
        list.push({ id: "screenshots", text: i18nc("@title:tab", "Screenshots") })
        list.push({ id: "log", text: i18nc("@title:tab", "Log") })
        if (!page.isLegacy)
            list.push({ id: "settings", text: i18nc("@title:tab", "Settings") })
        return list
    }
    readonly property int tabIndex: Math.max(0, page.tabs.findIndex(t => t.id === page.tab))
    readonly property string shownTab: page.tabs.length > 0 ? page.tabs[page.tabIndex].id : ""
    // The tabs shown so far: each is built the first time, then kept, so
    // switching back finds it as it was left.
    property var visitedTabs: []
    onShownTabChanged: {
        if (page.visitedTabs.indexOf(page.shownTab) < 0)
            page.visitedTabs = page.visitedTabs.concat([page.shownTab])
    }

    title: page.row ? page.row.name : (page.details ? page.details.name : "")
    padding: 0
    // The Log tab on one of the instance's log files (DevRoute.qml's
    // "logfile=<name>").
    function showLogFile(name) {
        page.tab = "log"
        if (logLoader.item)
            logLoader.item.showFile(name)
    }

    Component.onCompleted: page.shownTabChanged()
    Component.onDestruction: page.app.instancePageClosed(page)

    Instantiator {
        model: page.app.shell && page.app.shell.instancePageModel ? page.app.shell.instancePageModel : null
        delegate: QtObject {
            required property var model
            Component.onCompleted: page.row = model
            Component.onDestruction: if (page.row === model) page.row = null
        }
    }

    InstanceActions {
        id: instanceActions
        app: page.app
        instanceId: page.instanceId
        name: page.title
        iconKey: page.row ? page.row.iconKey : ""
        group: page.row ? page.row.group : ""
        running: page.running || page.launching
        canLaunch: !page.row || page.row.canLaunch !== false
    }
    actions: {
        var list = [instanceActions.play, instanceActions.openFolder]
        for (var i = 0; i < instanceActions.more.length; ++i)
            list.push(instanceActions.more[i])
        return list
    }

    header: QQC2.TabBar {
        id: tabBar
        currentIndex: page.tabIndex
        Repeater {
            model: page.tabs
            QQC2.TabButton {
                required property var modelData
                text: modelData.text
                width: implicitWidth
                onClicked: page.tab = modelData.id
            }
        }
        // A click moves currentIndex itself; a tab picked any other way
        // (navigate(), the dev route) moves it here.
        Connections {
            target: page
            function onTabIndexChanged() {
                tabBar.currentIndex = page.tabIndex
            }
        }
    }

    footer: LaunchFooter {
        status: page.previewLaunch ? page.previewLaunch.status : page.launching ? page.row.launchStatus : ""
        progress: page.previewLaunch ? page.previewLaunch.progress : page.launching ? page.row.launchProgress : -1
        onCancelRequested: page.app.stop(page.instanceId)
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: ["content", "worlds", "screenshots", "log", "settings"].indexOf(page.shownTab)

        Loader {
            active: page.visitedTabs.indexOf("content") >= 0
            sourceComponent: ContentTab {
                app: page.app
                details: page.details
                isCurrent: page.shownTab === "content" && page.isCurrentPage
            }
        }
        Loader {
            active: page.visitedTabs.indexOf("worlds") >= 0
            sourceComponent: WorldsTab {
                app: page.app
                details: page.details
            }
        }
        Loader {
            active: page.visitedTabs.indexOf("screenshots") >= 0
            sourceComponent: ScreenshotsTab {
                app: page.app
                details: page.details
            }
        }
        Loader {
            id: logLoader
            active: page.visitedTabs.indexOf("log") >= 0
            sourceComponent: LogTab {
                app: page.app
                details: page.details
                isCurrent: page.shownTab === "log" && page.isCurrentPage
            }
        }
        Item {
            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                icon.name: "configure"
                text: i18n("This instance's settings will be here soon")
                explanation: i18n("Java, memory and window settings for one instance are being rebuilt for KDE.")
            }
        }
    }
}
