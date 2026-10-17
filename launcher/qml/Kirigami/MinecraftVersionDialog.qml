// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD
import org.kde.kirigamiaddons.delegates as Delegates

/*
 * "Change Minecraft Version": the list of the game's versions to pick one
 * from for this instance. Snapshots and old versions are hidden until asked
 * for, as when a new instance is made.
 */
Kirigami.Dialog {
    id: dialog

    // InstanceDetails.
    property var details: null
    // Set when the dialog opens, not bound to details.minecraftVersions:
    // reading that builds the list and starts its download, and the page
    // opens without touching the network.
    property var versions: null

    title: i18nc("@title:dialog", "Change Minecraft Version")
    preferredWidth: Kirigami.Units.gridUnit * 26
    preferredHeight: Kirigami.Units.gridUnit * 30
    padding: Kirigami.Units.largeSpacing
    standardButtons: Kirigami.Dialog.Cancel
    customFooterActions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Change")
            icon.name: "dialog-ok-apply"
            enabled: list.currentIndex >= 0
            onTriggered: dialog.change(list.currentItem ? list.currentItem.versionId : "")
        }
    ]

    function change(versionId) {
        if (!dialog.details || !dialog.details.components || versionId.length === 0)
            return
        if (dialog.details.components.changeComponentVersion("net.minecraft", versionId))
            dialog.close()
    }

    // Whether the user has picked one: until then, the list arriving does
    // not leave its first (newest) version looking chosen.
    property bool picked: false

    onOpened: {
        dialog.picked = false
        list.currentIndex = -1
        if (dialog.details)
            dialog.versions = dialog.details.minecraftVersions
    }

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true
            QQC2.CheckBox {
                text: i18nc("@option:check", "Snapshots")
                checked: !!dialog.versions && dialog.versions.showSnapshots
                onToggled: if (dialog.versions) dialog.versions.showSnapshots = checked
            }
            QQC2.CheckBox {
                text: i18nc("@option:check", "Old versions")
                checked: !!dialog.versions && dialog.versions.showOldVersions
                onToggled: if (dialog.versions) dialog.versions.showOldVersions = checked
            }
        }

        ListArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                clip: true
                currentIndex: -1
                model: dialog.versions
                onCountChanged: if (!dialog.picked) list.currentIndex = -1
                delegate: Delegates.RoundedItemDelegate {
                    id: cell
                    required property int index
                    required property string versionId
                    required property string version
                    required property string type

                    width: ListView.view.width
                    text: cell.version
                    highlighted: ListView.isCurrentItem
                    contentItem: KD.TitleSubtitle {
                        title: cell.version
                        subtitle: cell.type === "release" ? "" : cell.type
                        selected: cell.highlighted
                        font: cell.font
                    }
                    onClicked: {
                        dialog.picked = true
                        list.currentIndex = cell.index
                    }
                    onDoubleClicked: dialog.change(cell.versionId)
                }

                QQC2.BusyIndicator {
                    anchors.centerIn: parent
                    visible: running
                    running: !!dialog.versions && dialog.versions.loading
                }
                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: !!dialog.versions && !dialog.versions.loading && dialog.versions.count === 0
                    icon.name: "edit-find"
                    text: i18n("No versions found")
                    explanation: dialog.versions && dialog.versions.error.length > 0
                                 ? dialog.versions.error
                                 : i18n("Try turning on snapshots or old versions.")
                }
            }
        }
    }
}
