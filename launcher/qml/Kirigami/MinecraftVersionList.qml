// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as Delegates

/*
 * The Minecraft versions New Instance offers, newest first, with their
 * release dates: NewInstanceController's minecraftVersions, which keeps
 * snapshots and old versions out unless they are asked for. Clicking a row
 * picks it.
 */
QQC2.ScrollView {
    id: view

    // NewInstanceController.
    required property var controller
    readonly property var versions: view.controller ? view.controller.minecraftVersions : null
    readonly property string selected: view.controller ? view.controller.selectedMinecraftVersion : ""

    function typeLabel(type) {
        switch (type) {
        case "snapshot": return i18nc("@info Minecraft version type", "Snapshot")
        case "old_beta": return i18nc("@info Minecraft version type", "Beta")
        case "old_alpha": return i18nc("@info Minecraft version type", "Alpha")
        default: return type
        }
    }
    // The @p n-th row's version, or "" while it is not listed.
    function versionAt(n) {
        list.positionViewAtIndex(n, ListView.Contain)
        var item = list.itemAtIndex(n)
        return item ? item.version : ""
    }

    QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

    ListView {
        id: list
        clip: true
        model: view.versions
        reuseItems: true

        delegate: Delegates.RoundedItemDelegate {
            id: row
            required property string version
            required property string type
            required property bool recommended
            required property var time
            readonly property bool dated: !!row.time && !isNaN(row.time)

            text: row.version
            highlighted: view.selected === row.version
            Accessible.description: row.dated ? row.time.toLocaleDateString(Qt.locale(), Locale.LongFormat) : ""
            onClicked: view.controller.selectMinecraftVersion(row.version)

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing
                QQC2.Label {
                    text: row.version
                    font.bold: row.highlighted
                    elide: Text.ElideRight
                    Layout.minimumWidth: 0
                }
                // Releases are the list's default and need no badge; a
                // snapshot or an old beta or alpha does.
                // A badge's own size is the theme's small font, which off
                // Plasma comes out larger than the text beside it.
                Kirigami.Badge {
                    visible: row.type !== "release"
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize
                    text: view.typeLabel(row.type)
                    type: row.type === "snapshot" ? Kirigami.Badge.Type.Warning : Kirigami.Badge.Type.Information
                }
                Kirigami.Badge {
                    visible: row.recommended
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize
                    text: i18nc("@info Minecraft version", "Recommended")
                    type: Kirigami.Badge.Type.Positive
                }
                Item {
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: row.dated ? row.time.toLocaleDateString(Qt.locale(), Locale.ShortFormat) : ""
                    color: Kirigami.Theme.disabledTextColor
                }
            }
        }

        QQC2.BusyIndicator {
            anchors.centerIn: parent
            visible: running
            running: !!view.versions && view.versions.loading
        }
        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: !!view.versions && !view.versions.loading && list.count === 0
            icon.name: view.versions && view.versions.error.length > 0 ? "network-disconnect" : "edit-find"
            text: view.versions && view.versions.error.length > 0
                  ? i18n("The version list could not be loaded")
                  : i18n("No versions to show")
            explanation: view.versions && view.versions.error.length > 0 ? view.versions.error : ""
        }
    }
}
