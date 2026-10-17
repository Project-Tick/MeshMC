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

    // The shell's plugin surfaces for this instance's page (anchor 1); the
    // Plugins tab is there only while a plugin has put something on it.
    readonly property var pluginModel: page.app.shell && typeof page.app.shell.pluginSurfaces === "function" && page.instanceId.length > 0
                                       ? page.app.shell.pluginSurfaces(1, page.instanceId) : null
    readonly property int pluginCount: pluginCounter.count
    readonly property bool isMinecraft: !!page.details && page.details.isMinecraft
    readonly property bool isManagedPack: !!page.details && !!page.details.managedPack
    // The Data Packs tab opens on this world when it was asked for from a
    // world's menu; -1 for the first.
    property int dataPacksWorld: -1

    // The everyday tabs come first and stay in view; `more` ones sit under
    // "More" (the bar shows the one open, so the user can see where they
    // are). An old-format instance can only be upgraded, so it has no
    // Content, Version or Settings.
    readonly property var tabs: {
        var list = []
        if (!page.isLegacy)
            list.push({ id: "content", text: i18nc("@title:tab", "Content") })
        list.push({ id: "worlds", text: i18nc("@title:tab", "Worlds") })
        list.push({ id: "screenshots", text: i18nc("@title:tab", "Screenshots") })
        list.push({ id: "log", text: i18nc("@title:tab", "Log") })
        if (!page.isLegacy)
            list.push({ id: "settings", text: i18nc("@title:tab", "Settings") })
        if (page.isManagedPack)
            list.push({ id: "managedpack", more: true, text: i18nc("@title:tab", "Modpack") })
        if (!page.isLegacy)
            list.push({ id: "version", more: true, text: i18nc("@title:tab", "Version") })
        if (page.isMinecraft) {
            list.push({ id: "datapacks", more: true, text: i18nc("@title:tab", "Data Packs") })
            list.push({ id: "servers", more: true, text: i18nc("@title:tab", "Servers") })
        }
        list.push({ id: "backups", more: true, text: i18nc("@title:tab", "Backups") })
        if (page.isMinecraft)
            list.push({ id: "gameoptions", more: true, text: i18nc("@title:tab", "Game Options") })
        list.push({ id: "notes", more: true, text: i18nc("@title:tab", "Notes") })
        if (page.pluginCount > 0)
            list.push({ id: "plugins", more: true,
                        text: page.pluginCount === 1 && pluginCounter.firstTitle.length > 0
                              ? pluginCounter.firstTitle : i18nc("@title:tab", "Plugins") })
        return list
    }
    // What the tab bar shows: the everyday tabs, and the open one when it
    // is among the rest.
    readonly property var barTabs: page.tabs.filter(t => !t.more || t.id === page.shownTab)
    readonly property var moreTabs: page.tabs.filter(t => t.more && t.id !== page.shownTab)
    readonly property int tabIndex: Math.max(0, page.tabs.findIndex(t => t.id === page.tab))
    readonly property string shownTab: page.tabs.length > 0 ? page.tabs[page.tabIndex].id : ""
    readonly property int barIndex: Math.max(0, page.barTabs.findIndex(t => t.id === page.shownTab))
    // The tabs shown so far: each is built the first time, then kept, so
    // switching back finds it as it was left.
    property var visitedTabs: []
    onShownTabChanged: {
        if (page.visitedTabs.indexOf(page.shownTab) < 0)
            page.visitedTabs = page.visitedTabs.concat([page.shownTab])
    }

    title: page.row ? page.row.name : (page.details ? page.details.name : "")
    // The instance's icon and its loader's logo beside the name.
    titleDelegate: InstanceTitle {
        entry: page.row
        title: page.title
        logos: page.app.shell ? page.app.shell.brandLogos : null
    }
    padding: 0
    // The Log tab on one of the instance's log files (DevRoute.qml's
    // "logfile=<name>").
    function showLogFile(name) {
        page.tab = "log"
        if (logLoader.item)
            logLoader.item.showFile(name)
    }

    // The open tab's own state for review snapshots (DevRoute.qml's
    // "review=<name>"): a dialog, a selection. False until the tab is built
    // and knows that name.
    function review(name) {
        var loader = tabStack.children[tabStack.currentIndex]
        var tabItem = loader ? loader.item : null
        return !!tabItem && typeof tabItem.review === "function" && tabItem.review(name) === true
    }

    // Back from Add Content: the Content tab on the kind just browsed.
    function showContentKind(kind) {
        if (contentLoader.item)
            contentLoader.item.showKind(kind)
    }

    // The Content tab's Check for Updates or Export List dialog, open: for
    // review snapshots (DevRoute.qml's "updates" and "export"). False until
    // the tab is there.
    function openContentDialog(which) {
        page.tab = "content"
        var content = contentLoader.item
        if (!content || !content.content)
            return false
        // Asked once: a check that cannot start says why, and asking again
        // would only say it again.
        if (which === "updates")
            content.checkUpdates()
        else
            content.exportList()
        return true
    }

    // Installs from Add Content, kept here so they go on after Add Content
    // closes, and the dialog that lists what one brings along.
    readonly property alias contentInstaller: installer
    ContentInstaller {
        id: installer
        app: page.app
        details: page.details
        onReviewRequested: (review, key) => installReview.show(review, key)
    }
    InstallReviewDialog {
        id: installReview
        onInstallStarted: (watcher, key, title) => installer.track(key, watcher, title)
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

    // The everyday tabs, the open one if it is among the rest, and "More"
    // with the others.
    header: RowLayout {
        spacing: 0

        QQC2.TabBar {
            id: tabBar
            Layout.fillWidth: true
            currentIndex: page.barIndex
            Repeater {
                id: barButtons
                model: page.barTabs
                // The bar ignores a currentIndex past its last button, and
                // an index it already holds. An open tab from under More joins
                // the buttons after the index moved: set it again once the
                // last one is there.
                onItemAdded: (index, item) => {
                    if (index === barButtons.count - 1) {
                        tabBar.currentIndex = -1
                        tabBar.currentIndex = page.barIndex
                    }
                }
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
                function onBarIndexChanged() {
                    tabBar.currentIndex = page.barIndex
                }
            }
        }
        QQC2.ToolButton {
            id: moreTabsButton
            Layout.fillHeight: true
            visible: page.moreTabs.length > 0
            text: i18nc("@action:button more tabs of an instance", "More")
            icon.name: "arrow-down"
            LayoutMirroring.enabled: true
            LayoutMirroring.childrenInherit: true
            onClicked: moreTabsMenu.popup(moreTabsButton, 0, moreTabsButton.height)
        }
    }

    QQC2.Menu {
        id: moreTabsMenu
        Instantiator {
            model: page.moreTabs
            delegate: QQC2.MenuItem {
                required property var modelData
                text: modelData.text
                onTriggered: page.tab = modelData.id
            }
            onObjectAdded: (index, object) => moreTabsMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => moreTabsMenu.removeItem(object)
        }
    }

    // Counts the plugin surfaces on this page, which decide the Plugins tab.
    Instantiator {
        id: pluginCounter
        readonly property string firstTitle: count > 0 && objectAt(0) ? objectAt(0).title : ""
        model: page.pluginModel
        delegate: QtObject {
            required property string title
        }
    }

    footer: LaunchFooter {
        status: page.previewLaunch ? page.previewLaunch.status : page.launching ? page.row.launchStatus : ""
        progress: page.previewLaunch ? page.previewLaunch.progress : page.launching ? page.row.launchProgress : -1
        onCancelRequested: page.app.stop(page.instanceId)
    }

    StackLayout {
        id: tabStack
        anchors.fill: parent
        currentIndex: ["content", "worlds", "screenshots", "log", "settings",
                       "managedpack", "version", "datapacks", "servers", "backups",
                       "gameoptions", "notes", "plugins"].indexOf(page.shownTab)

        Loader {
            id: contentLoader
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
                onDataPacksRequested: (row) => {
                    page.dataPacksWorld = row
                    page.tab = "datapacks"
                }
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
        Loader {
            id: settingsLoader
            active: page.visitedTabs.indexOf("settings") >= 0
            sourceComponent: InstanceSettingsTab {
                app: page.app
                details: page.details
                running: page.running || page.launching
            }
        }
        Loader {
            active: page.visitedTabs.indexOf("managedpack") >= 0
            sourceComponent: ManagedPackTab {
                app: page.app
                details: page.details
                // The pack has just been replaced: this page shows the old one.
                onUpdated: page.app.showSection("library")
            }
        }
        Loader {
            active: page.visitedTabs.indexOf("version") >= 0
            sourceComponent: VersionTab {
                app: page.app
                details: page.details
            }
        }
        Loader {
            active: page.visitedTabs.indexOf("datapacks") >= 0
            sourceComponent: DataPacksTab {
                app: page.app
                details: page.details
                selectedWorldRow: page.dataPacksWorld
            }
        }
        Loader {
            active: page.visitedTabs.indexOf("servers") >= 0
            sourceComponent: ServersTab {
                app: page.app
                details: page.details
            }
        }
        Loader {
            active: page.visitedTabs.indexOf("backups") >= 0
            sourceComponent: BackupsTab {
                app: page.app
                details: page.details
            }
        }
        Loader {
            active: page.visitedTabs.indexOf("gameoptions") >= 0
            sourceComponent: GameOptionsTab {
                app: page.app
                details: page.details
                running: page.running || page.launching
            }
        }
        Loader {
            active: page.visitedTabs.indexOf("notes") >= 0
            sourceComponent: NotesTab {
                app: page.app
                details: page.details
            }
        }
        Loader {
            active: page.visitedTabs.indexOf("plugins") >= 0
            sourceComponent: PluginsTab {
                app: page.app
                details: page.details
                model: page.pluginModel
            }
        }
    }
}
