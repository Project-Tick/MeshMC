// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * One skin in the skin page's grid: its front-and-back sprite and name,
 * highlighted while picked, with a tick on the one the account wears. A
 * right click (or a long press) asks for its menu. A default skin that has
 * not downloaded yet shows a busy indicator in its place.
 */
QQC2.ItemDelegate {
    id: tile

    property string name
    property url spriteUrl
    property bool selected: false
    property bool worn: false
    property bool ready: true
    property bool hasMenu: false

    signal menuRequested()

    implicitWidth: Kirigami.Units.gridUnit * 5
    implicitHeight: Kirigami.Units.gridUnit * 6
    highlighted: tile.selected
    enabled: tile.ready
    text: tile.name
    Accessible.description: tile.worn ? i18nc("@info:status", "Worn now") : ""
    onPressAndHold: if (tile.hasMenu) tile.menuRequested()

    TapHandler {
        acceptedButtons: Qt.RightButton
        enabled: tile.hasMenu
        onTapped: tile.menuRequested()
    }

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Image {
                anchors.fill: parent
                source: tile.ready ? tile.spriteUrl : ""
                fillMode: Image.PreserveAspectFit
                smooth: false
                asynchronous: true
            }
            QQC2.BusyIndicator {
                anchors.centerIn: parent
                visible: !tile.ready
                running: visible
            }
            Kirigami.Icon {
                anchors.right: parent.right
                anchors.top: parent.top
                visible: tile.worn
                source: "checkmark"
                width: Kirigami.Units.iconSizes.small
                height: width
                color: Kirigami.Theme.positiveTextColor
            }
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: tile.name
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
