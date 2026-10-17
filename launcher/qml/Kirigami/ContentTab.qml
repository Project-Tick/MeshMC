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
 * The instance's installed content -- mods, resource packs, shader packs
 * and, where the instance has them, core mods, texture packs and data packs
 * -- one kind at a time, each file with a switch to turn it on or off.
 *
 * The toolbar picks the kind and searches it, adds files and opens the
 * folder. Mods are locked while the game runs, as they always were: the
 * files are open, and a change would only apply next time. Packs the game
 * reloads by itself stay editable.
 *
 * Downloading from Modrinth and CurseForge, updates and the list export
 * come with the content browser in a later phase.
 */
Item {
    id: tab

    required property var app
    property var details: null
    // Whether this tab is the one on screen (it then answers Ctrl+F).
    property bool isCurrent: false

    // InstanceContentController.
    readonly property var content: tab.details ? tab.details.content : null
    property string kind: "mods"

    // Per kind: its name in the picker, what an empty list says, and the
    // lock message while the game runs.
    readonly property var kindInfo: ({
        mods: {
            text: i18nc("@item:inlistbox", "Mods"),
            empty: i18n("No mods yet"),
            locked: i18n("The game is running. Mods can be changed once it closes.")
        },
        coremods: {
            text: i18nc("@item:inlistbox", "Core Mods"),
            empty: i18n("No core mods yet"),
            locked: i18n("The game is running. Core mods can be changed once it closes.")
        },
        resourcepacks: {
            text: i18nc("@item:inlistbox", "Resource Packs"),
            empty: i18n("No resource packs yet"),
            locked: i18n("The game is running. Resource packs can be changed once it closes.")
        },
        shaderpacks: {
            text: i18nc("@item:inlistbox", "Shader Packs"),
            empty: i18n("No shader packs yet"),
            locked: i18n("The game is running. Shader packs can be changed once it closes.")
        },
        texturepacks: {
            text: i18nc("@item:inlistbox", "Texture Packs"),
            empty: i18n("No texture packs yet"),
            locked: i18n("The game is running. Texture packs can be changed once it closes.")
        },
        datapacks: {
            text: i18nc("@item:inlistbox", "Data Packs"),
            empty: i18n("No data packs yet"),
            locked: i18n("The game is running. Data packs can be changed once it closes.")
        }
    })
    readonly property var current: tab.kindInfo[tab.kind] || tab.kindInfo.mods
    readonly property var kinds: tab.content
                                 ? tab.content.kinds.map(k => ({ value: k, text: (tab.kindInfo[k] || { text: k }).text }))
                                 : []
    // The searchable list of the current kind (ContentFilterModel).
    readonly property var listModel: {
        if (!tab.content)
            return null
        tab.content.kinds // asked again when a kind appears or goes
        return tab.content.model(tab.kind)
    }
    readonly property bool unlocked: {
        if (!tab.content)
            return false
        tab.content.running
        return tab.content.changesAllowed(tab.kind)
    }
    readonly property int totalCount: tab.listModel ? tab.listModel.totalCount : 0

    // A kind that went away (data packs switched off) falls back to mods.
    onKindsChanged: {
        if (tab.kinds.length > 0 && !tab.kinds.some(k => k.value === tab.kind))
            tab.kind = "mods"
    }

    function fileName(url) {
        var path = decodeURIComponent(String(url))
        return path.slice(path.lastIndexOf("/") + 1)
    }
    function installFiles(urls) {
        var failed = []
        for (var i = 0; i < urls.length; ++i) {
            if (!tab.content.install(tab.kind, urls[i].toString()))
                failed.push(tab.fileName(urls[i]))
        }
        if (failed.length > 0)
            tab.app.notify(i18n("Could not add %1.", failed.join(", ")))
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        QQC2.ToolBar {
            Layout.fillWidth: true
            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.ComboBox {
                    model: tab.kinds
                    textRole: "text"
                    valueRole: "value"
                    currentIndex: Math.max(0, tab.kinds.findIndex(k => k.value === tab.kind))
                    onActivated: tab.kind = currentValue
                    Accessible.name: i18nc("@label:listbox", "Kind of content")
                }
                Kirigami.SearchField {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 16
                    placeholderText: i18nc("@info:placeholder", "Search…")
                    focusSequences: tab.isCurrent ? [StandardKey.Find] : []
                    onTextChanged: if (tab.listModel) tab.listModel.filterText = text
                    // The kind changed: its list starts unsearched.
                    Connections {
                        target: tab
                        function onKindChanged() {
                            searchField.text = ""
                        }
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                QQC2.ToolButton {
                    text: i18nc("@action:button", "Add Files…")
                    icon.name: "list-add"
                    enabled: tab.unlocked
                    onClicked: addFiles.open()
                }
                QQC2.ToolButton {
                    text: i18nc("@action:button", "Open Folder")
                    icon.name: "document-open-folder"
                    onClicked: tab.app.call("openPath", tab.content.dir(tab.kind))
                }
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: !tab.unlocked && !!tab.content
            position: Kirigami.InlineMessage.Header
            type: Kirigami.MessageType.Information
            text: tab.current.locked
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: tab.kind === "mods" && !!tab.content && !tab.content.hasModLoader && tab.totalCount > 0
            position: Kirigami.InlineMessage.Header
            type: Kirigami.MessageType.Warning
            text: i18n("This instance has no mod loader, so its mods are not loaded.")
        }

        ListArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                clip: true
                model: tab.listModel
                delegate: ContentRow {
                    width: ListView.view.width
                    unlocked: tab.unlocked
                    onToggleRequested: (key, enabled) => tab.content.setEnabled(tab.kind, [key], enabled)
                    onMenuRequested: (parent, position) => tab.openMenu(model, parent, position)
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: list.count === 0
                    icon.name: tab.totalCount > 0 ? "edit-find" : "package-x-generic"
                    text: tab.totalCount > 0 ? i18n("Nothing matches your search") : tab.current.empty
                    explanation: tab.totalCount > 0 ? "" : i18n("Add files here, or drop them into the folder.")
                }
            }
        }
    }

    // One file's menu: switch it, its project page, remove it.
    function openMenu(entry, parent, position) {
        menuComponent.createObject(tab, { entry: entry }).popup(parent, position)
    }
    Component {
        id: menuComponent
        Components.ConvergentContextMenu {
            id: menu
            required property var entry
            onClosed: menu.destroy()
            Kirigami.Action {
                text: menu.entry.enabled ? i18nc("@action:inmenu", "Disable") : i18nc("@action:inmenu", "Enable")
                icon.name: menu.entry.enabled ? "dialog-cancel" : "dialog-ok-apply"
                enabled: tab.unlocked
                onTriggered: tab.content.setEnabled(tab.kind, [menu.entry.key], !menu.entry.enabled)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Open Project Page")
                icon.name: "internet-services"
                visible: (menu.entry.homepage || "").length > 0
                onTriggered: Qt.openUrlExternally(menu.entry.homepage)
            }
            Kirigami.Action {
                separator: true
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Remove…")
                icon.name: "edit-delete"
                enabled: tab.unlocked
                onTriggered: {
                    removeDialog.key = menu.entry.key
                    removeDialog.name = menu.entry.name || menu.entry.fileName
                    removeDialog.open()
                }
            }
        }
    }

    Kirigami.PromptDialog {
        id: removeDialog
        property string key
        property string name
        title: i18nc("@title:dialog", "Remove File")
        subtitle: i18n("Remove “%1” from this instance? The file is deleted.", removeDialog.name)
        dialogType: Kirigami.PromptDialog.Warning
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Remove")
                icon.name: "edit-delete"
                onTriggered: {
                    removeDialog.close()
                    tab.content.remove(tab.kind, [removeDialog.key])
                }
            }
        ]
    }

    Dialogs.FileDialog {
        id: addFiles
        title: i18nc("@title:dialog", "Add Files")
        fileMode: Dialogs.FileDialog.OpenFiles
        onAccepted: tab.installFiles(selectedFiles)
    }
}
