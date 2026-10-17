// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * One cape the account can wear, or "No cape": its front, cut out of the
 * 64x32 texture (a 10x16 strip at (1, 1)) by drawing the whole texture
 * larger inside a clipped box, and its name. A cape whose texture has not
 * arrived yet shows a placeholder in its place.
 */
QQC2.ItemDelegate {
    id: tile

    property string name
    property url capeUrl
    property bool selected: false
    property bool noCape: false
    readonly property bool hasCape: tile.capeUrl.toString().length > 0

    implicitWidth: Kirigami.Units.gridUnit * 5
    implicitHeight: Kirigami.Units.gridUnit * 6
    highlighted: tile.selected
    text: tile.name

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Item {
                id: strip
                anchors.centerIn: parent
                visible: tile.hasCape
                height: parent.height
                width: height * 10 / 16
                clip: true
                readonly property real unit: width / 10
                Image {
                    source: tile.capeUrl
                    asynchronous: true
                    smooth: false
                    width: 64 * strip.unit
                    height: 32 * strip.unit
                    x: -strip.unit
                    y: -strip.unit
                }
            }
            Kirigami.Icon {
                anchors.centerIn: parent
                visible: !tile.hasCape
                source: tile.noCape ? "edit-none" : "image-loading"
                width: Kirigami.Units.iconSizes.medium
                height: width
                color: Kirigami.Theme.disabledTextColor
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
