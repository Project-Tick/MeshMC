// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components

/*
 * Data packs of one of the instance's worlds (saves/<world>/datapacks): a
 * world to pick, the packs in it each with a switch, and Add From File to
 * put a .zip in. The Worlds tab's "Data Packs" action opens this on that
 * world (`selectedWorldRow`); otherwise the first world is shown.
 */
Item {
    id: tab

    required property var app
    property var details: null
    // Set from outside (the Worlds tab's per-row action) to jump straight
    // to a world; -1 shows whichever the picker has (the first at first).
    property int selectedWorldRow: -1

    // WorldList, and the controller of the world shown (WorldDataPacksController).
    readonly property var worldsModel: tab.details ? tab.details.worlds : null
    readonly property var controller: tab.details ? tab.details.worldDataPacks : null
    readonly property bool ready: !!tab.controller && tab.controller.ready
    readonly property bool unlocked: !!tab.controller && tab.controller.unlocked

    function openWorld(row) {
        if (!tab.controller || row < 0)
            return
        picker.currentIndex = row
        tab.controller.openForWorld(row)
    }
    function openDefault() {
        if (tab.selectedWorldRow >= 0)
            tab.openWorld(tab.selectedWorldRow)
        else if (tab.worldsModel && picker.count > 0)
            tab.openWorld(0)
    }
    onSelectedWorldRowChanged: if (tab.selectedWorldRow >= 0) tab.openWorld(tab.selectedWorldRow)
    onControllerChanged: tab.openDefault()
    // For review snapshots (DevRoute.qml's "review=<name>").
    function review(name) {
        if (name === "add")
            addPack.open()
        else if (name.indexOf("world") === 0)
            tab.openWorld(parseInt(name.slice(5)))
        else
            return false
        return true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: picker.count > 0

        TabToolBar {
            Layout.fillWidth: true
            actions: [
                Kirigami.Action {
                    text: i18nc("@action:button", "Add From File…")
                    icon.name: "list-add"
                    enabled: tab.unlocked && tab.ready
                    onTriggered: addPack.open()
                },
                Kirigami.Action {
                    text: i18nc("@action:button", "Open Folder")
                    icon.name: "document-open-folder"
                    enabled: tab.ready
                    onTriggered: tab.app.call("openPath", tab.controller.directory)
                }
            ]

            QQC2.Label {
                text: i18nc("@label:listbox", "World:")
            }
            QQC2.ComboBox {
                id: picker
                Layout.preferredWidth: Kirigami.Units.gridUnit * 14
                model: tab.worldsModel
                textRole: "name"
                Accessible.name: i18nc("@label:listbox", "World")
                onActivated: (index) => tab.openWorld(index)
                // The world list may still be loading when the controller
                // arrives: open the default world as soon as there is one.
                onCountChanged: if (tab.controller && !tab.controller.ready) tab.openDefault()
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: !tab.unlocked && !!tab.controller
            position: Kirigami.InlineMessage.Header
            type: Kirigami.MessageType.Information
            text: i18n("The game is running. Data packs can be changed once it closes.")
        }

        ListArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                clip: true
                currentIndex: -1
                model: tab.controller ? tab.controller.model : null
                delegate: ContentRow {
                    id: row
                    width: ListView.view.width
                    unlocked: tab.unlocked
                    onToggleRequested: (key, enabled) => tab.controller.setEnabled(row.index, enabled)
                    onMenuRequested: (parent, position) => tab.openMenu(row.index, row.model, parent, position)
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: list.count === 0
                    icon.name: "package-x-generic"
                    text: i18n("No data packs")
                    explanation: i18n("Data packs change this world's recipes, loot and more. Add a .zip to get started.")
                    helpfulAction: Kirigami.Action {
                        text: i18nc("@action:button", "Add From File…")
                        icon.name: "list-add"
                        enabled: tab.unlocked && tab.ready
                        onTriggered: addPack.open()
                    }
                }
            }
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: picker.count === 0
        icon.name: "package-x-generic"
        text: i18n("No worlds yet")
        explanation: i18n("Data packs live inside a world's own folder, so create a world first.")
    }

    // One pack's menu: switch it, remove it.
    function openMenu(row, entry, parent, position) {
        menuComponent.createObject(tab, { row: row, entry: entry }).popup(parent, position)
    }
    Component {
        id: menuComponent
        Components.ConvergentContextMenu {
            id: menu
            required property int row
            required property var entry
            onClosed: menu.destroy()
            Kirigami.Action {
                text: menu.entry.enabled ? i18nc("@action:inmenu", "Disable") : i18nc("@action:inmenu", "Enable")
                icon.name: menu.entry.enabled ? "dialog-cancel" : "dialog-ok-apply"
                enabled: tab.unlocked
                onTriggered: tab.controller.setEnabled(menu.row, !menu.entry.enabled)
            }
            Kirigami.Action {
                separator: true
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Remove…")
                icon.name: "edit-delete"
                enabled: tab.unlocked
                onTriggered: {
                    removeDialog.row = menu.row
                    removeDialog.subtitle = i18n("Remove “%1” from this world? The file is deleted.", menu.entry.name)
                    removeDialog.open()
                }
            }
        }
    }

    ConfirmActionDialog {
        id: removeDialog
        property int row: -1
        title: i18nc("@title:dialog", "Remove Data Pack")
        onConfirmed: if (tab.controller) tab.controller.remove(removeDialog.row)
    }

    Dialogs.FileDialog {
        id: addPack
        title: i18nc("@title:dialog", "Add Data Pack")
        nameFilters: [i18n("Data pack files (*.zip)"), i18n("All files (*)")]
        onAccepted: if (tab.controller) tab.controller.install(selectedFile.toString())
    }
}
