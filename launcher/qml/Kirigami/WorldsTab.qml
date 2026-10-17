// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import org.kde.coreaddons as KCoreAddons
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD
import org.kde.kirigamiaddons.components as Components
import org.kde.kirigamiaddons.delegates as Delegates

/*
 * The instance's single-player worlds: each with its own icon, name, game
 * mode, day and when it was last played. A world's actions -- data packs,
 * copy its seed, open it in MCEdit, rename, copy, reset its icon, delete --
 * are behind its "More" button and right-click; Add World unpacks a .zip
 * (picked, or dropped onto the tab) into the worlds folder.
 *
 * Changing a world while the game runs can corrupt it, so every action that
 * changes one asks "Continue anyway?" first instead of being unavailable.
 * Deleting one asks too -- a world is hours of someone's play, and there is
 * no undo.
 */
Item {
    id: tab

    required property var app
    property var details: null

    readonly property var worlds: tab.details ? tab.details.worlds : null
    readonly property bool unlocked: !!tab.details && tab.details.contentChangesAllowed
    // Asks the instance page for the Data Packs tab on world `row`.
    signal dataPacksRequested(int row)

    // The TaskWatcher of a copy or an install in progress, if any -- both
    // run off the GUI thread (a world can run into gigabytes).
    property var watcher: null
    readonly property bool busy: !!tab.watcher && tab.watcher.running
    // What the watcher is doing, for the footer and the messages.
    property string busyText: i18n("Copying world…")
    property string failText: i18n("The world could not be copied.")
    property string doneText: i18n("World copied.")
    // The action waiting on the "game is running" question.
    property var pendingAction: null

    // A different instance: whatever the tab was doing belonged to the
    // previous one.
    onDetailsChanged: tab.watcher = null

    // Runs @p action now, or after "Continue anyway?" while the game runs.
    function guarded(action) {
        if (tab.unlocked) {
            action()
            return
        }
        tab.pendingAction = action
        runningDialog.open()
    }

    // Starts installing the first of @p fileUrls that is a Minecraft world.
    // One install runs at a time -- they would share the one progress
    // footer -- so a drop of several adds the first and says so.
    function addWorlds(fileUrls) {
        if (!tab.details || tab.busy)
            return
        for (var i = 0; i < fileUrls.length; ++i) {
            var started = tab.details.installWorld(String(fileUrls[i]))
            if (!started)
                continue
            tab.busyText = i18n("Adding world…")
            tab.failText = i18n("The world could not be added.")
            tab.doneText = fileUrls.length > 1 ? i18n("World added. Add the others one at a time.")
                                               : i18n("World added.")
            tab.watcher = started
            return
        }
        tab.app.notify(i18n("That is not a Minecraft world. Pick a .zip with a world in it."))
    }

    // A world's icon file as a URL Image can load; "" when it has none.
    function fileUrl(path) {
        if (!path || String(path).length === 0)
            return ""
        var p = String(path).replace(/\\/g, "/")
        return "file://" + (p.charAt(0) === "/" ? "" : "/") + p
    }

    // The game modes worth showing. WorldList reports a level.dat without
    // one as "Undefined" (or "Unknown (n)"), which says nothing to a
    // player, so anything else is left out.
    readonly property var knownModes: [i18nc("GameType", "Survival"), i18nc("GameType", "Creative"),
                                       i18nc("GameType", "Adventure"), i18nc("GameType", "Spectator")]

    // For review snapshots (DevRoute.qml's "review=<name>").
    function review(name) {
        if (name === "rename" && list.count > 0)
            tab.askRename(0, list.itemAtIndex(0).displayName)
        else if (name === "delete" && list.count > 0)
            tab.askDelete(0, list.itemAtIndex(0).displayName)
        else if (name === "running")
            runningDialog.open()
        else
            return false
        return true
    }

    function askRename(row, name) {
        tab.guarded(() => {
            renameDialog.row = row
            renameDialog.ask(name)
        })
    }
    function askCopy(row, name) {
        tab.guarded(() => {
            copyDialog.row = row
            copyDialog.ask(i18n("%1 (copy)", name))
        })
    }
    function askDelete(row, name) {
        tab.guarded(() => {
            deleteDialog.row = row
            deleteDialog.subtitle = i18n("Delete the world “%1”? It cannot be recovered from the launcher.", name)
            deleteDialog.open()
        })
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabToolBar {
            Layout.fillWidth: true
            actions: [
                Kirigami.Action {
                    text: i18nc("@action:button", "Add World…")
                    icon.name: "list-add"
                    enabled: !!tab.details && !tab.busy
                    onTriggered: tab.guarded(() => addWorld.open())
                },
                Kirigami.Action {
                    text: i18nc("@action:button", "Refresh")
                    icon.name: "view-refresh"
                    onTriggered: tab.details.refreshWorlds()
                },
                Kirigami.Action {
                    text: i18nc("@action:button", "Open Folder")
                    icon.name: "document-open-folder"
                    onTriggered: tab.app.call("openPath", tab.details.worldsDir)
                }
            ]
        }

        WatcherMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            watcher: tab.watcher
            failText: tab.failText
        }

        ListArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                clip: true
                currentIndex: -1
                model: tab.worlds
                delegate: Delegates.RoundedItemDelegate {
                    id: row
                    required property int index
                    required property string name
                    required property string folder
                    required property var gameMode
                    required property var lastPlayed
                    required property var iconFile
                    required property var dayCount
                    required property var seed

                    // The folder's own name, the title of a world without a
                    // level name. `folder` is the full path.
                    readonly property string displayName: row.name.length > 0 ? row.name : String(row.folder).split(/[\\/]/).pop()
                    readonly property string mode: String(row.gameMode || "")

                    width: ListView.view.width
                    text: row.displayName
                    contentItem: RowLayout {
                        spacing: Kirigami.Units.largeSpacing

                        // The world's own icon.png, kept crisp like the
                        // pixel picture it is. Without one, a neutral globe:
                        // a landscape would pass for a real view of the
                        // world, and two worlds would share one.
                        Item {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                            Layout.preferredHeight: Kirigami.Units.iconSizes.huge
                            Image {
                                id: worldIcon
                                anchors.fill: parent
                                source: tab.fileUrl(row.iconFile)
                                sourceSize: Qt.size(width * 2, height * 2)
                                fillMode: Image.PreserveAspectCrop
                                smooth: false
                                asynchronous: true
                            }
                            Kirigami.Icon {
                                anchors.fill: parent
                                anchors.margins: Kirigami.Units.smallSpacing
                                visible: worldIcon.status !== Image.Ready
                                source: "map-globe"
                            }
                        }
                        KD.TitleSubtitle {
                            Layout.fillWidth: true
                            title: row.displayName
                            subtitle: {
                                var parts = []
                                if (tab.knownModes.indexOf(row.mode) >= 0)
                                    parts.push(row.mode)
                                if (row.dayCount !== undefined && row.dayCount !== null)
                                    parts.push(i18nc("@info a world's age in game days", "Day %1", String(row.dayCount)))
                                if (row.lastPlayed)
                                    parts.push(i18nc("@info %1 is a date", "Played %1",
                                                     KCoreAddons.Format.formatRelativeDateTime(row.lastPlayed, Locale.ShortFormat)))
                                return parts.join(" · ")
                            }
                            font: row.font
                            selected: row.highlighted
                        }
                        QQC2.ToolButton {
                            id: moreButton
                            enabled: !tab.busy
                            icon.name: "overflow-menu"
                            display: QQC2.AbstractButton.IconOnly
                            text: i18nc("@action:button", "More")
                            QQC2.ToolTip.text: text
                            QQC2.ToolTip.visible: hovered
                            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                            onClicked: tab.openMenu(row, moreButton, Qt.point(0, moreButton.height))
                        }
                    }
                    onDoubleClicked: tab.app.call("openPath", row.folder)

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        enabled: !tab.busy
                        onTapped: (eventPoint) => tab.openMenu(row, row, eventPoint.position)
                    }
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: list.count === 0
                    icon.name: "map-globe"
                    text: i18n("No worlds yet")
                    explanation: i18n("Worlds you create in single player show up here. You can also add one from a .zip.")
                    helpfulAction: Kirigami.Action {
                        text: i18nc("@action:button", "Add World…")
                        icon.name: "list-add"
                        enabled: !!tab.details && !tab.busy
                        onTriggered: tab.guarded(() => addWorld.open())
                    }
                }
            }
        }

        ProgressFooter {
            Layout.fillWidth: true
            active: tab.busy
            status: tab.watcher ? (tab.watcher.status || tab.busyText) : ""
            progress: tab.watcher ? tab.watcher.progress : -1
        }
    }

    // Worlds dropped from the file manager. Only the first is
    // added; the rest would fight over the one progress footer.
    DropArea {
        id: drop
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: (event) => {
            if (event.hasUrls) {
                // The event is gone once this handler returns, but
                // the question may still be open.
                var urls = event.urls
                tab.guarded(() => tab.addWorlds(urls))
            }
        }

        Rectangle {
            anchors.fill: parent
            visible: drop.containsDrag
            color: Qt.alpha(Kirigami.Theme.highlightColor, 0.2)
            border.width: 2
            border.color: Kirigami.Theme.highlightColor

            QQC2.Label {
                anchors.centerIn: parent
                text: i18n("Drop a world here to add it")
                font.bold: true
            }
        }
    }

    Connections {
        target: tab.watcher
        ignoreUnknownSignals: true
        function onFinished(ok) {
            if (ok)
                tab.app.notify(tab.doneText)
        }
    }

    // One world's menu.
    function openMenu(row, parent, position) {
        menuComponent.createObject(tab, {
            world: {
                index: row.index,
                name: row.displayName,
                folder: row.folder,
                seed: row.seed,
                hasIcon: !!row.iconFile
            }
        }).popup(parent, position)
    }
    Component {
        id: menuComponent
        Components.ConvergentContextMenu {
            id: menu
            required property var world
            // Whether the MCEdit setting points at a usable MCEdit: asked
            // each time, since it is a launcher setting that can change.
            readonly property bool mcedit: !!tab.details && tab.details.mceditConfigured()
            onClosed: menu.destroy()

            Kirigami.Action {
                text: i18nc("@action:inmenu", "Data Packs")
                icon.name: "package-x-generic"
                // An old-format instance has no Data Packs tab.
                visible: !!tab.details && tab.details.isMinecraft
                onTriggered: tab.dataPacksRequested(menu.world.index)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Copy Seed")
                icon.name: "edit-copy"
                visible: menu.world.seed !== undefined && menu.world.seed !== null
                onTriggered: {
                    clipboard.copyText(String(menu.world.seed))
                    tab.app.notify(i18n("Seed copied."))
                }
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Open in MCEdit")
                icon.name: "document-edit"
                visible: menu.mcedit
                onTriggered: tab.guarded(() => {
                    var problem = tab.details.openWorldInMCEdit(menu.world.index)
                    if (problem.length > 0)
                        tab.app.notify(problem)
                })
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Open Folder")
                icon.name: "document-open-folder"
                onTriggered: tab.app.call("openPath", menu.world.folder)
            }
            Kirigami.Action {
                separator: true
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Rename…")
                icon.name: "edit-rename"
                onTriggered: tab.askRename(menu.world.index, menu.world.name)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Copy…")
                icon.name: "edit-copy"
                onTriggered: tab.askCopy(menu.world.index, menu.world.name)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Reset Icon")
                icon.name: "edit-clear"
                enabled: menu.world.hasIcon
                onTriggered: tab.guarded(() => tab.details.resetWorldIcon(menu.world.index))
            }
            Kirigami.Action {
                separator: true
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Delete…")
                icon.name: "edit-delete"
                onTriggered: tab.askDelete(menu.world.index, menu.world.name)
            }
        }
    }

    ConfirmActionDialog {
        id: deleteDialog
        property int row: -1
        title: i18nc("@title:dialog", "Delete World")
        actionText: i18nc("@action:button", "Delete World")
        onConfirmed: if (tab.details) tab.details.deleteWorld(deleteDialog.row)
    }

    TextPromptDialog {
        id: renameDialog
        property int row: -1
        title: i18nc("@title:dialog", "Rename World")
        confirmText: i18nc("@action:button", "Rename")
        onSubmitted: (text) => {
            if (!tab.details || tab.details.renameWorld(renameDialog.row, text))
                renameDialog.close()
            else
                renameDialog.error = i18n("That name cannot be used.")
        }
    }

    TextPromptDialog {
        id: copyDialog
        property int row: -1
        title: i18nc("@title:dialog", "Copy World")
        confirmText: i18nc("@action:button", "Copy")
        onSubmitted: (text) => {
            var started = tab.details ? tab.details.copyWorld(copyDialog.row, text) : null
            if (started) {
                tab.busyText = i18n("Copying world…")
                tab.failText = i18n("The world could not be copied.")
                tab.doneText = i18n("World copied.")
                tab.watcher = started
                copyDialog.close()
            } else {
                copyDialog.error = i18n("That name cannot be used.")
            }
        }
    }

    ConfirmActionDialog {
        id: runningDialog
        title: i18nc("@title:dialog", "Minecraft Is Running")
        subtitle: i18n("Changing a world while Minecraft is running can damage it. Continue anyway?")
        actionText: i18nc("@action:button", "Continue Anyway")
        actionIcon: "dialog-warning"
        onConfirmed: {
            var action = tab.pendingAction
            tab.pendingAction = null
            if (action)
                action()
        }
    }

    Dialogs.FileDialog {
        id: addWorld
        title: i18nc("@title:dialog", "Add World")
        nameFilters: [i18n("World archives (*.zip)"), i18n("All files (*)")]
        onAccepted: tab.addWorlds([selectedFile])
    }

    Clipboard {
        id: clipboard
    }
}
