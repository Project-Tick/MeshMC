// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD
import org.kde.kirigamiaddons.components as Components
import org.kde.kirigamiaddons.delegates as Delegates

/*
 * Backups of the instance: zip snapshots of its whole folder, with its
 * worlds and mods. Creating, restoring, exporting and importing one all run
 * off the GUI thread behind a TaskWatcher (BackupController), so the tab
 * shows one progress footer for whichever is in flight. A backup cannot be
 * restored while the game runs: the files are open.
 */
Item {
    id: tab

    required property var app
    property var details: null

    // BackupController.
    readonly property var controller: tab.details ? tab.details.backups : null
    readonly property bool gameRunning: !!tab.controller && tab.controller.running
    // The TaskWatcher of whichever action is in flight, if any.
    property var watcher: null
    readonly property bool busy: !!tab.watcher && tab.watcher.running
    // Set when an action refuses to start (restoring while the game runs):
    // the watcher stays null then, so this is how the tab says why.
    property string actionError: ""

    // BackupController's timestampText is "yyyy-MM-dd HH:mm:ss" local time.
    function whenOf(timestampText) {
        var when = new Date(String(timestampText).replace(" ", "T"))
        return isNaN(when.getTime()) ? null : when
    }
    // What a backup is called. One made without a label is named after its
    // creation time ("2026-09-18_09-00-00.zip"), which reads better as a
    // date; a labelled one keeps its label. ".zip" is never part of it.
    function backupTitle(name, timestampText) {
        var bare = String(name).replace(/\.zip$/i, "")
        if (!/^\d{4}-\d{2}-\d{2}[_ T-]\d{2}[-.:]\d{2}[-.:]\d{2}$/.test(bare))
            return bare
        var when = tab.whenOf(timestampText)
        return when ? i18nc("@title a backup made on a date at a time", "%1, %2",
                            when.toLocaleDateString(Qt.locale(), Locale.LongFormat),
                            when.toLocaleTimeString(Qt.locale(), Locale.ShortFormat))
                    : bare
    }
    // Whether the title is the creation time: the subtitle then keeps to the size.
    function isTimeTitle(name) {
        return /^\d{4}-\d{2}-\d{2}[_ T-]\d{2}[-.:]\d{2}[-.:]\d{2}$/.test(String(name).replace(/\.zip$/i, ""))
    }
    function startWatching(started) {
        tab.actionError = ""
        tab.watcher = started
    }
    // For review snapshots (DevRoute.qml's "review=<name>").
    function review(name) {
        if (name === "create")
            labelDialog.ask()
        else
            return false
        return true
    }

    // A different instance's controller: whatever the tab was doing
    // belonged to the previous one.
    onControllerChanged: {
        tab.watcher = null
        tab.actionError = ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabToolBar {
            Layout.fillWidth: true
            actions: [
                Kirigami.Action {
                    text: i18nc("@action:button", "Create Backup…")
                    icon.name: "archive-insert"
                    enabled: !tab.busy
                    onTriggered: labelDialog.ask()
                },
                Kirigami.Action {
                    text: i18nc("@action:button", "Import…")
                    icon.name: "document-import"
                    enabled: !tab.busy
                    onTriggered: importFile.open()
                }
            ]
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: tab.gameRunning
            position: Kirigami.InlineMessage.Header
            type: Kirigami.MessageType.Information
            text: i18n("The game is running. Backups can be restored once it closes.")
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: tab.actionError.length > 0
            position: Kirigami.InlineMessage.Header
            type: Kirigami.MessageType.Error
            text: tab.actionError
        }
        WatcherMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            watcher: tab.watcher
            failText: i18n("The backup action failed.")
        }

        ListArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                clip: true
                currentIndex: -1
                model: tab.controller ? tab.controller.model : null
                delegate: Delegates.RoundedItemDelegate {
                    id: row
                    required property int index
                    required property string name
                    required property string timestampText
                    required property string sizeText

                    readonly property string title: tab.backupTitle(row.name, row.timestampText)

                    width: ListView.view.width
                    text: row.title
                    contentItem: RowLayout {
                        spacing: Kirigami.Units.largeSpacing

                        KD.IconTitleSubtitle {
                            Layout.fillWidth: true
                            title: row.title
                            subtitle: {
                                if (tab.isTimeTitle(row.name))
                                    return row.sizeText
                                var when = tab.whenOf(row.timestampText)
                                return i18nc("@info a backup's age and size", "%1, %2",
                                             when ? when.toLocaleString(Qt.locale(), Locale.ShortFormat) : row.timestampText,
                                             row.sizeText)
                            }
                            selected: row.highlighted
                            font: row.font
                            icon.name: "package-x-generic"
                            icon.width: Kirigami.Units.iconSizes.medium
                            icon.height: Kirigami.Units.iconSizes.medium
                        }
                        QQC2.Button {
                            text: i18nc("@action:button", "Restore")
                            icon.name: "edit-undo"
                            enabled: !tab.busy && !tab.gameRunning
                            QQC2.ToolTip.visible: hovered && tab.gameRunning
                            QQC2.ToolTip.text: i18n("Close the game before restoring a backup.")
                            onClicked: tab.askRestore(row.index, row.title)
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
                            onClicked: tab.openMenu(row.index, row.title, moreButton, Qt.point(0, moreButton.height))
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        enabled: !tab.busy
                        onTapped: (eventPoint) => tab.openMenu(row.index, row.title, row, eventPoint.position)
                    }
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: list.count === 0
                    icon.name: "archive-insert"
                    text: i18n("No backups yet")
                    explanation: i18n("A backup is a zip of this whole instance, including its worlds and mods, that you can restore later.")
                    helpfulAction: Kirigami.Action {
                        text: i18nc("@action:button", "Create Backup…")
                        icon.name: "archive-insert"
                        enabled: !tab.busy
                        onTriggered: labelDialog.ask()
                    }
                }
            }
        }

        ProgressFooter {
            Layout.fillWidth: true
            active: tab.busy
            status: tab.watcher ? (tab.watcher.status || tab.watcher.title || i18n("Working…")) : ""
            progress: tab.watcher ? tab.watcher.progress : -1
        }
    }

    function askRestore(row, title) {
        restoreDialog.row = row
        restoreDialog.subtitle = i18n("Replace this instance's current contents with the backup “%1”? This cannot be undone.", title)
        restoreDialog.open()
    }

    // One backup's menu: restore, export, delete.
    function openMenu(row, title, parent, position) {
        menuComponent.createObject(tab, { row: row, backupName: title }).popup(parent, position)
    }
    Component {
        id: menuComponent
        Components.ConvergentContextMenu {
            id: menu
            required property int row
            required property string backupName
            onClosed: menu.destroy()
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Restore…")
                icon.name: "edit-undo"
                enabled: !tab.gameRunning
                onTriggered: tab.askRestore(menu.row, menu.backupName)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Export…")
                icon.name: "document-export"
                onTriggered: {
                    exportFile.row = menu.row
                    exportFile.open()
                }
            }
            Kirigami.Action {
                separator: true
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Delete…")
                icon.name: "edit-delete"
                onTriggered: {
                    deleteDialog.row = menu.row
                    deleteDialog.subtitle = i18n("Delete the backup “%1”? This cannot be undone.", menu.backupName)
                    deleteDialog.open()
                }
            }
        }
    }

    // The label is optional: a backup without one is named after its time.
    Kirigami.PromptDialog {
        id: labelDialog
        title: i18nc("@title:dialog", "Create Backup")
        subtitle: i18n("Give it a label to find it by, such as “before updating mods”. You can leave it empty.")
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        function ask() {
            labelField.text = ""
            labelDialog.open()
            labelField.forceActiveFocus()
        }
        onAccepted: {
            if (tab.controller)
                tab.startWatching(tab.controller.createBackup(labelField.text.trim()))
        }

        QQC2.TextField {
            id: labelField
            placeholderText: i18n("Label")
            Accessible.name: i18nc("@label:textbox", "Label")
            onAccepted: labelDialog.accept()
        }
    }
    ConfirmActionDialog {
        id: restoreDialog
        property int row: -1
        title: i18nc("@title:dialog", "Restore Backup")
        actionText: i18nc("@action:button", "Restore")
        actionIcon: "edit-undo"
        onConfirmed: {
            if (!tab.controller)
                return
            var started = tab.controller.restoreBackup(restoreDialog.row)
            if (started)
                tab.startWatching(started)
            else
                tab.actionError = i18n("Close the game before restoring a backup.")
        }
    }
    ConfirmActionDialog {
        id: deleteDialog
        property int row: -1
        title: i18nc("@title:dialog", "Delete Backup")
        actionText: i18nc("@action:button", "Delete")
        onConfirmed: if (tab.controller) tab.startWatching(tab.controller.deleteBackup(deleteDialog.row))
    }

    Dialogs.FileDialog {
        id: importFile
        title: i18nc("@title:dialog", "Import Backup")
        nameFilters: [i18n("Zip files (*.zip)"), i18n("All files (*)")]
        onAccepted: if (tab.controller) tab.startWatching(tab.controller.importBackup(selectedFile.toString(), ""))
    }
    Dialogs.FileDialog {
        id: exportFile
        property int row: -1
        title: i18nc("@title:dialog", "Export Backup")
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: [i18n("Zip files (*.zip)")]
        onAccepted: if (tab.controller) tab.startWatching(tab.controller.exportBackup(exportFile.row, selectedFile.toString()))
    }
}
