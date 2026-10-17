// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD
import org.kde.kirigamiaddons.components as Components
import org.kde.kirigamiaddons.delegates as Delegates

/*
 * The instance's servers.dat, laid out like the game's own server list: one
 * row per server with the favicon the game last saw, its name and address,
 * and Join to launch the game straight into it. Edit, reorder and remove
 * are behind the row's "More" button and right-click. Locked while the game
 * runs -- it rewrites the file as it quits.
 */
Item {
    id: tab

    required property var app
    property var details: null

    // ServersListModel.
    readonly property var model: tab.details ? tab.details.servers : null
    readonly property bool unlocked: !!tab.model && !tab.model.locked

    // Launches the game into @p address (needs an account, like Play).
    function join(address) {
        if (tab.app.sendToAccountsIfNone())
            return
        if (tab.app.shell && typeof tab.app.shell.joinServer === "function")
            tab.app.shell.joinServer(tab.details.instanceId, address)
    }
    // For review snapshots (DevRoute.qml's "review=<name>").
    function review(name) {
        if (name === "add")
            serverDialog.edit(-1, "", "", 0)
        else if (name === "edit" && list.count > 0)
            serverDialog.edit(0, list.itemAtIndex(0).name, list.itemAtIndex(0).address, list.itemAtIndex(0).acceptTextures)
        else
            return false
        return true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabToolBar {
            Layout.fillWidth: true
            actions: [
                Kirigami.Action {
                    text: i18nc("@action:button", "Add Server…")
                    icon.name: "list-add"
                    enabled: tab.unlocked
                    onTriggered: serverDialog.edit(-1, "", "", 0)
                },
                Kirigami.Action {
                    text: i18nc("@action:button", "Open Folder")
                    icon.name: "document-open-folder"
                    onTriggered: tab.app.call("openPath", tab.details.serversDir)
                }
            ]
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: !tab.unlocked && !!tab.model
            position: Kirigami.InlineMessage.Header
            type: Kirigami.MessageType.Information
            text: i18n("The game is running. The server list can be changed once it closes.")
        }

        ListArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                objectName: "serversList"
                clip: true
                currentIndex: -1
                model: tab.model
                delegate: Delegates.RoundedItemDelegate {
                    id: row
                    required property int index
                    required property string name
                    required property string address
                    required property int acceptTextures
                    // The favicon the game saved when it last pinged the
                    // server, as a data: URL; "" if it never has.
                    required property string iconUrl

                    readonly property string title: row.name.length > 0 ? row.name : i18n("Minecraft Server")
                    readonly property bool joinable: row.address.trim().length > 0

                    width: ListView.view.width
                    text: row.title
                    contentItem: RowLayout {
                        spacing: Kirigami.Units.largeSpacing

                        // The favicon in the game's own pixels; the server
                        // icon until the game has seen the server.
                        Item {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.large
                            Layout.preferredHeight: Kirigami.Units.iconSizes.large
                            Image {
                                id: favicon
                                anchors.fill: parent
                                source: row.iconUrl
                                sourceSize: Qt.size(width, height)
                                fillMode: Image.PreserveAspectFit
                                smooth: false
                            }
                            Kirigami.Icon {
                                anchors.fill: parent
                                visible: favicon.status !== Image.Ready
                                source: "network-server"
                            }
                        }
                        KD.TitleSubtitle {
                            Layout.fillWidth: true
                            title: row.title
                            subtitle: row.address.length > 0 ? row.address : i18n("No address yet")
                            selected: row.highlighted
                            font: row.font
                        }
                        QQC2.Button {
                            text: i18nc("@action:button", "Join")
                            icon.name: "media-playback-start"
                            enabled: row.joinable
                            onClicked: tab.join(row.address)
                        }
                        QQC2.ToolButton {
                            id: moreButton
                            icon.name: "overflow-menu"
                            display: QQC2.AbstractButton.IconOnly
                            text: i18nc("@action:button", "More")
                            QQC2.ToolTip.text: text
                            QQC2.ToolTip.visible: hovered
                            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                            onClicked: tab.openMenu(row, moreButton, Qt.point(0, moreButton.height))
                        }
                    }
                    onDoubleClicked: if (tab.unlocked) tab.openMenuEdit(row)

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (eventPoint) => tab.openMenu(row, row, eventPoint.position)
                    }
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: list.count === 0
                    icon.name: "network-server"
                    text: i18n("No servers yet")
                    explanation: i18n("Servers you add in the game show up here. You can also add one yourself.")
                    helpfulAction: Kirigami.Action {
                        text: i18nc("@action:button", "Add Server…")
                        icon.name: "list-add"
                        enabled: tab.unlocked
                        onTriggered: serverDialog.edit(-1, "", "", 0)
                    }
                }
            }
        }
    }

    function openMenuEdit(row) {
        serverDialog.edit(row.index, row.name, row.address, row.acceptTextures)
    }

    // One server's menu: edit, move, remove.
    function openMenu(row, parent, position) {
        menuComponent.createObject(tab, { server: row }).popup(parent, position)
    }
    Component {
        id: menuComponent
        Components.ConvergentContextMenu {
            id: menu
            required property var server
            onClosed: menu.destroy()
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Join")
                icon.name: "media-playback-start"
                enabled: menu.server.joinable
                onTriggered: tab.join(menu.server.address)
            }
            Kirigami.Action {
                separator: true
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Edit…")
                icon.name: "document-edit"
                enabled: tab.unlocked
                onTriggered: tab.openMenuEdit(menu.server)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Move Up")
                icon.name: "go-up"
                enabled: tab.unlocked && menu.server.index > 0
                onTriggered: tab.model.moveUp(menu.server.index)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Move Down")
                icon.name: "go-down"
                enabled: tab.unlocked && menu.server.index < list.count - 1
                onTriggered: tab.model.moveDown(menu.server.index)
            }
            Kirigami.Action {
                separator: true
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Remove…")
                icon.name: "edit-delete"
                enabled: tab.unlocked
                onTriggered: {
                    removeDialog.row = menu.server.index
                    removeDialog.subtitle = i18n("Remove “%1” from this instance's server list?",
                                                 menu.server.name.length > 0 ? menu.server.name : menu.server.address)
                    removeDialog.open()
                }
            }
        }
    }

    ConfirmActionDialog {
        id: removeDialog
        property int row: -1
        title: i18nc("@title:dialog", "Remove Server")
        onConfirmed: if (tab.model) tab.model.removeServer(removeDialog.row)
    }

    ServerDialog {
        id: serverDialog
        model: tab.model
        onSaved: (row) => list.positionViewAtIndex(row, ListView.Contain)
    }
}
