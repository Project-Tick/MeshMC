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
 * "Install a Mod Loader": pick a loader (Fabric, Quilt, Forge, ...) and a
 * version of it for this instance's Minecraft. Installing one that cannot
 * run beside a loader already on turns that one off, which is asked about
 * first.
 */
Kirigami.Dialog {
    id: dialog

    // LoaderInstaller (InstanceDetails.loaderInstaller).
    property var installer: null
    // Loader uid to open on; empty opens the first one in the list.
    property string preselectUid: ""

    title: i18nc("@title:dialog", "Install a Mod Loader")
    preferredWidth: Kirigami.Units.gridUnit * 30
    preferredHeight: Kirigami.Units.gridUnit * 30
    padding: Kirigami.Units.largeSpacing
    standardButtons: Kirigami.Dialog.Cancel
    customFooterActions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Install")
            icon.name: "download"
            enabled: list.currentIndex >= 0
            onTriggered: dialog.install(list.currentItem ? list.currentItem.versionId : "")
        }
    ]

    function install(versionId) {
        if (!dialog.installer || versionId.length === 0)
            return
        if (dialog.installer.conflictName.length > 0) {
            var picked = dialog.installer.loaders.find(l => l.uid === dialog.installer.selectedUid)
            conflict.versionId = versionId
            conflict.subtitle = i18n("%1 and %2 hook into the same parts of the game and cannot run together. Installing this turns %1 off.",
                                     dialog.installer.conflictName, picked ? picked.brandName : i18n("This loader"))
            conflict.open()
            return
        }
        if (dialog.installer.install(versionId))
            dialog.close()
    }

    onOpened: {
        list.currentIndex = -1
        if (!dialog.installer)
            return
        var uid = dialog.preselectUid.length > 0 ? dialog.preselectUid
                : (dialog.installer.loaders.length > 0 ? dialog.installer.loaders[0].uid : "")
        if (uid.length > 0)
            dialog.installer.selectLoader(uid)
    }

    Connections {
        target: dialog.installer
        function onSelectedUidChanged() {
            list.currentIndex = -1
        }
    }
    // The newest version is picked once the list arrives.
    Connections {
        target: dialog.installer ? dialog.installer.versions : null
        function onCountChanged() {
            if (list.currentIndex < 0 && dialog.installer.versions.count > 0)
                list.currentIndex = 0
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.TabBar {
            Layout.fillWidth: true
            currentIndex: dialog.installer
                          ? Math.max(0, dialog.installer.loaders.findIndex(l => l.uid === dialog.installer.selectedUid)) : 0
            Repeater {
                model: dialog.installer ? dialog.installer.loaders : []
                QQC2.TabButton {
                    required property var modelData
                    text: modelData.brandName
                    width: implicitWidth
                    onClicked: dialog.installer.selectLoader(modelData.uid)
                }
            }
        }

        ListArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                clip: true
                currentIndex: -1
                visible: !!dialog.installer && dialog.installer.supported
                model: dialog.installer ? dialog.installer.versions : null
                delegate: Delegates.RoundedItemDelegate {
                    id: cell
                    required property int index
                    required property string versionId
                    required property string version
                    required property bool recommended

                    width: ListView.view.width
                    text: cell.version
                    highlighted: ListView.isCurrentItem
                    contentItem: KD.TitleSubtitle {
                        title: cell.version
                        subtitle: cell.recommended ? i18nc("@info a loader version", "Recommended") : ""
                        selected: cell.highlighted
                        font: cell.font
                    }
                    onClicked: list.currentIndex = cell.index
                    onDoubleClicked: dialog.install(cell.versionId)
                }

                QQC2.BusyIndicator {
                    anchors.centerIn: parent
                    visible: running
                    running: !!dialog.installer && !!dialog.installer.versions && dialog.installer.versions.loading
                }
                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: !!dialog.installer && dialog.installer.supported && !!dialog.installer.versions
                             && !dialog.installer.versions.loading && dialog.installer.versions.count === 0
                    icon.name: "edit-find"
                    text: i18n("No versions found")
                    explanation: dialog.installer && dialog.installer.versions.error.length > 0
                                 ? dialog.installer.versions.error
                                 : i18n("Nothing is published for this Minecraft version yet.")
                }
            }
            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: !!dialog.installer && !dialog.installer.supported
                icon.name: "dialog-warning"
                text: i18n("Not compatible")
                explanation: dialog.installer ? dialog.installer.unsupportedReason : ""
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: !!dialog.installer && dialog.installer.installedVersion.length > 0
            text: dialog.installer ? i18n("Already installed: %1", dialog.installer.installedVersion) : ""
            color: Kirigami.Theme.disabledTextColor
            elide: Text.ElideRight
        }
    }

    ConfirmActionDialog {
        id: conflict
        property string versionId
        title: i18nc("@title:dialog", "Turn Off the Other Loader?")
        actionText: i18nc("@action:button", "Install Anyway")
        actionIcon: "download"
        onConfirmed: {
            if (dialog.installer.install(conflict.versionId))
                dialog.close()
        }
    }
}
