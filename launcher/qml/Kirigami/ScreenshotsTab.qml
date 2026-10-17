// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components

/*
 * The instance's screenshots, newest first, as thumbnails. A click opens
 * one in the system's image viewer. Ticking the box that shows on a
 * hovered tile (or Ctrl-clicking it) selects it; once anything is
 * selected, clicking a tile toggles it instead, and the toolbar offers
 * Upload and Delete for the whole selection. Each tile's menu (also on
 * right-click) has Open, Copy Image, Rename, Upload and Delete.
 *
 * Upload goes to Imgur (one picture: a link to it; several: an album) and
 * puts the link on the clipboard; builds without an Imgur client id leave
 * it out. Thumbnails are decoded off the UI thread by the "screenshot"
 * image provider.
 */
Item {
    id: tab

    required property var app
    property var details: null

    // ScreenshotListModel: name, path, url, modified, size; remove(),
    // rename(), copyImage(), pathAt().
    readonly property var shots: tab.details ? tab.details.screenshots : null
    readonly property int count: tab.shots ? tab.shots.count : 0
    // The Imgur uploader: `available`, `busy`, upload().
    readonly property var uploader: tab.details ? tab.details.screenshotUploader : null
    readonly property bool canUpload: !!tab.uploader && tab.uploader.available

    readonly property int thumbnailWidth: Kirigami.Units.gridUnit * 12
    // Thumbnails are cut to the shape of a typical game window.
    readonly property real thumbnailAspect: 16 / 9

    // The selected screenshots, by path: {path: true}. Paths rather than
    // rows because rows move when a file is added, renamed or deleted.
    property var selected: ({})
    property int selectedCount: 0
    // Set by the Upload actions, so only this tab reacts to the result.
    property bool uploading: false

    // What a screenshot is called under its thumbnail. The game names them
    // after the moment they were taken ("2026-09-22_18.00.12.png"), which
    // reads better as a date and time; one renamed to anything else keeps
    // its name. Never the file extension.
    function captionOf(name) {
        var bare = tab.baseName(name)
        var m = /^(\d{4})-(\d{2})-(\d{2})_(\d{2})\.(\d{2})\.(\d{2})$/.exec(bare)
        if (!m)
            return bare
        var taken = new Date(Number(m[1]), Number(m[2]) - 1, Number(m[3]), Number(m[4]), Number(m[5]), Number(m[6]))
        return i18nc("@info a screenshot's date and time", "%1, %2",
                     taken.toLocaleDateString(Qt.locale(), Locale.ShortFormat),
                     taken.toLocaleTimeString(Qt.locale(), Locale.ShortFormat))
    }
    // A file name without its extension, for the rename box.
    function baseName(name) {
        var dot = name.lastIndexOf(".")
        return dot > 0 ? name.slice(0, dot) : name
    }

    function isSelected(path) {
        return tab.selected[path] === true
    }
    function setSelection(next) {
        tab.selected = next
        tab.selectedCount = Object.keys(next).length
    }
    function toggleSelected(path) {
        var next = Object.assign({}, tab.selected)
        if (next[path])
            delete next[path]
        else
            next[path] = true
        tab.setSelection(next)
    }
    function clearSelection() {
        tab.setSelection({})
    }
    function selectAll() {
        var next = {}
        for (var i = 0; tab.shots && i < tab.shots.count; ++i)
            next[tab.shots.pathAt(i)] = true
        tab.setSelection(next)
    }
    function rowOf(path) {
        for (var i = 0; tab.shots && i < tab.shots.count; ++i) {
            if (tab.shots.pathAt(i) === path)
                return i
        }
        return -1
    }
    // The selected paths, or just @p path when it is outside the selection
    // (a right-click on another tile).
    function targets(path) {
        return tab.isSelected(path) ? Object.keys(tab.selected) : [path]
    }

    function upload(paths) {
        if (!tab.canUpload || tab.uploader.busy)
            return
        tab.uploading = true
        tab.uploader.upload(paths)
    }
    function copyImage(row) {
        tab.app.notify(tab.shots.copyImage(row) ? i18n("Image copied.") : i18n("The image could not be copied."))
    }
    function askDelete(paths) {
        deleteDialog.paths = paths
        deleteDialog.subtitle = paths.length === 1
                ? i18n("Move this screenshot to the trash?")
                : i18np("Move %1 screenshot to the trash?", "Move %1 screenshots to the trash?", paths.length)
        deleteDialog.open()
    }
    // Moves @p paths to the trash, last row first so the earlier rows keep
    // their numbers while it goes.
    function removeAll(paths) {
        var rows = []
        for (var i = 0; i < paths.length; ++i) {
            var row = tab.rowOf(paths[i])
            if (row >= 0)
                rows.push(row)
        }
        rows.sort((a, b) => b - a)
        var failed = 0
        for (var j = 0; j < rows.length; ++j) {
            if (!tab.shots.remove(rows[j]))
                failed++
        }
        tab.clearSelection()
        if (failed > 0)
            tab.app.notify(i18n("Some screenshots could not be deleted."))
    }
    function askRename(path, name) {
        renameDialog.path = path
        renameDialog.ask(tab.baseName(name))
    }

    // For review snapshots (DevRoute.qml's "review=<name>").
    function review(name) {
        if (name === "select")
            tab.selectAll()
        else if (name === "rename" && tab.count > 0)
            tab.askRename(tab.shots.pathAt(0), tab.shots.pathAt(0).split("/").pop())
        else if (name === "delete" && tab.count > 0)
            tab.askDelete([tab.shots.pathAt(0)])
        else
            return false
        return true
    }

    Connections {
        target: tab.uploader
        ignoreUnknownSignals: true
        function onUploaded(url, count) {
            if (!tab.uploading)
                return
            tab.uploading = false
            clipboard.copyText(url)
            tab.app.notify(count > 1 ? i18n("Album link copied to your clipboard.")
                                     : i18n("Link copied to your clipboard."),
                           i18nc("@action:button", "Open"), () => Qt.openUrlExternally(url))
        }
        function onFailed(message) {
            if (!tab.uploading)
                return
            tab.uploading = false
            tab.app.notify(message)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabToolBar {
            Layout.fillWidth: true
            actions: [
                Kirigami.Action {
                    text: i18nc("@action:button", "Select All")
                    icon.name: "edit-select-all"
                    visible: tab.selectedCount === 0 && tab.count > 1
                    onTriggered: tab.selectAll()
                },
                Kirigami.Action {
                    text: i18nc("@action:button", "Clear Selection")
                    icon.name: "edit-select-none"
                    visible: tab.selectedCount > 0
                    onTriggered: tab.clearSelection()
                },
                Kirigami.Action {
                    text: tab.uploading ? i18nc("@action:button", "Uploading…") : i18nc("@action:button", "Upload to Imgur")
                    icon.name: "cloud-upload"
                    visible: tab.selectedCount > 0 && tab.canUpload
                    enabled: !!tab.uploader && !tab.uploader.busy
                    onTriggered: tab.upload(Object.keys(tab.selected))
                },
                Kirigami.Action {
                    text: i18nc("@action:button", "Delete")
                    icon.name: "edit-delete"
                    visible: tab.selectedCount > 0
                    onTriggered: tab.askDelete(Object.keys(tab.selected))
                },
                Kirigami.Action {
                    text: i18nc("@action:button", "Open Folder")
                    icon.name: "document-open-folder"
                    onTriggered: tab.app.call("openPath", tab.details.screenshotsDir)
                }
            ]

            QQC2.Label {
                visible: tab.selectedCount > 0
                text: i18np("%1 selected", "%1 selected", tab.selectedCount)
                color: Kirigami.Theme.disabledTextColor
            }
        }

        ListArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            GridView {
                id: grid
                clip: true
                model: tab.shots
                // Columns fill the width, one more once there is room for
                // another thumbnail.
                readonly property int columns: Math.max(1, Math.floor(width / tab.thumbnailWidth))
                cellWidth: Math.floor(width / columns)
                // The thumbnail, with a line for the caption below.
                cellHeight: Math.round(cellWidth / tab.thumbnailAspect) + Kirigami.Units.gridUnit * 2

                delegate: QQC2.ItemDelegate {
                    id: cell
                    required property int index
                    required property string name
                    required property string path
                    required property string url

                    readonly property bool picked: tab.isSelected(cell.path)

                    width: grid.cellWidth
                    height: grid.cellHeight
                    highlighted: cell.picked
                    hoverEnabled: true
                    Accessible.name: cell.name
                    // A click opens the picture -- or, once something is
                    // selected, ticks it; a Ctrl-click always ticks.
                    TapHandler {
                        acceptedModifiers: Qt.NoModifier
                        onTapped: {
                            if (tab.selectedCount > 0)
                                tab.toggleSelected(cell.path)
                            else
                                Qt.openUrlExternally(cell.url)
                        }
                    }
                    TapHandler {
                        acceptedModifiers: Qt.ControlModifier
                        onTapped: tab.toggleSelected(cell.path)
                    }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (eventPoint) => tab.openMenu(cell, cell, eventPoint.position)
                    }

                    contentItem: ColumnLayout {
                        spacing: Kirigami.Units.smallSpacing

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Image {
                                anchors.fill: parent
                                source: "image://screenshot/" + encodeURIComponent(cell.path)
                                sourceSize.width: width
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                clip: true
                            }
                            // The tick that selects a tile: on hover, and on
                            // every tile once a selection exists.
                            QQC2.CheckBox {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                visible: cell.hovered || tab.selectedCount > 0
                                checked: cell.picked
                                Accessible.name: i18nc("@option:check", "Select %1", cell.name)
                                onToggled: {
                                    tab.toggleSelected(cell.path)
                                    // toggled() replaced the binding with
                                    // the user's value; put it back.
                                    checked = Qt.binding(() => cell.picked)
                                }
                            }
                            QQC2.ToolButton {
                                id: tileMore
                                anchors.right: parent.right
                                anchors.top: parent.top
                                visible: cell.hovered
                                icon.name: "overflow-menu"
                                display: QQC2.AbstractButton.IconOnly
                                text: i18nc("@action:button", "More")
                                QQC2.ToolTip.text: text
                                QQC2.ToolTip.visible: hovered
                                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                                onClicked: tab.openMenu(cell, tileMore, Qt.point(0, tileMore.height))
                            }
                        }
                        QQC2.Label {
                            Layout.fillWidth: true
                            text: tab.captionOf(cell.name)
                            elide: Text.ElideMiddle
                            horizontalAlignment: Text.AlignHCenter
                            font: Kirigami.Theme.smallFont
                        }
                    }
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: grid.count === 0
                    icon.name: "folder-pictures"
                    text: i18n("No screenshots yet")
                    explanation: i18n("Press F2 in the game to take one.")
                }
            }
        }
    }

    // One screenshot's menu; with a selection, Upload and Delete act on all
    // of it when the screenshot is part of it.
    function openMenu(cell, parent, position) {
        menuComponent.createObject(tab, {
            shot: { index: cell.index, name: cell.name, path: cell.path, url: cell.url }
        }).popup(parent, position)
    }
    Component {
        id: menuComponent
        Components.ConvergentContextMenu {
            id: menu
            required property var shot
            readonly property bool many: tab.selectedCount > 1 && tab.isSelected(menu.shot.path)
            onClosed: menu.destroy()

            Kirigami.Action {
                text: i18nc("@action:inmenu", "Open")
                icon.name: "document-open"
                onTriggered: Qt.openUrlExternally(menu.shot.url)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Copy Image")
                icon.name: "edit-copy"
                onTriggered: tab.copyImage(menu.shot.index)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Rename…")
                icon.name: "edit-rename"
                onTriggered: tab.askRename(menu.shot.path, menu.shot.name)
            }
            Kirigami.Action {
                text: menu.many ? i18nc("@action:inmenu", "Upload Selected to Imgur") : i18nc("@action:inmenu", "Upload to Imgur")
                icon.name: "cloud-upload"
                visible: tab.canUpload
                enabled: !!tab.uploader && !tab.uploader.busy
                onTriggered: tab.upload(tab.targets(menu.shot.path))
            }
            Kirigami.Action {
                separator: true
            }
            Kirigami.Action {
                text: menu.many ? i18nc("@action:inmenu", "Delete Selected…") : i18nc("@action:inmenu", "Delete…")
                icon.name: "edit-delete"
                onTriggered: tab.askDelete(tab.targets(menu.shot.path))
            }
        }
    }

    ConfirmActionDialog {
        id: deleteDialog
        property var paths: []
        title: i18nc("@title:dialog", "Delete Screenshots")
        actionText: i18nc("@action:button", "Move to Trash")
        onConfirmed: tab.removeAll(deleteDialog.paths)
    }

    TextPromptDialog {
        id: renameDialog
        property string path
        title: i18nc("@title:dialog", "Rename Screenshot")
        confirmText: i18nc("@action:button", "Rename")
        onSubmitted: (text) => {
            var row = tab.rowOf(renameDialog.path)
            if (row >= 0 && tab.shots.rename(row, text)) {
                // The selection is by path, which just changed.
                tab.clearSelection()
                renameDialog.close()
            } else {
                renameDialog.error = i18n("That name cannot be used, or another screenshot already has it.")
            }
        }
    }

    Clipboard {
        id: clipboard
    }
}
