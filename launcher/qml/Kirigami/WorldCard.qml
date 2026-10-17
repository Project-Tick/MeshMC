// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.coreaddons as KCoreAddons
import org.kde.kirigami as Kirigami

import "artwork.js" as Artwork

/*
 * One of Home's recent worlds: the save's own icon (its icon.png, as the
 * game's world list shows it; a biome picture for a world that has none),
 * its name, the instance it is in and when it was last played. A click
 * opens that instance on its Worlds tab.
 *
 * The properties are RecentWorldsModel's roles.
 */
Kirigami.AbstractCard {
    id: world

    required property var app
    required property string worldName
    required property string folderName
    required property string iconUrl
    required property var lastPlayed
    required property string instanceId
    required property string instanceName

    readonly property bool ownIcon: world.iconUrl.length > 0
    readonly property real lastPlayedMs: Number(world.lastPlayed) || 0

    showClickFeedback: true
    Accessible.name: world.worldName
    onClicked: world.app.openInstance(world.instanceId, "worlds")

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        // Square, as the game's own world list shows it.
        Image {
            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
            Layout.preferredHeight: Kirigami.Units.iconSizes.huge
            source: world.ownIcon ? world.iconUrl : Artwork.worldUrl(world.folderName)
            // A save's icon is a small screenshot, kept at its own size and
            // scaled up unsmoothed, which keeps it sharp; the biome pictures
            // are larger and decoded at twice the size shown.
            sourceSize: world.ownIcon ? Qt.size(Kirigami.Units.iconSizes.huge, Kirigami.Units.iconSizes.huge)
                                      : Qt.size(Kirigami.Units.iconSizes.huge * 2, Kirigami.Units.iconSizes.huge * 2)
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            smooth: !world.ownIcon
            mipmap: !world.ownIcon
            clip: true
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            QQC2.Label {
                Layout.fillWidth: true
                text: world.worldName
                font.bold: true
                elide: Text.ElideRight
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: world.instanceName
                elide: Text.ElideRight
                opacity: 0.7
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: world.lastPlayedMs > 0
                text: world.lastPlayedMs > 0
                      ? KCoreAddons.Format.formatRelativeDateTime(new Date(world.lastPlayedMs), Locale.ShortFormat) : ""
                elide: Text.ElideRight
                opacity: 0.7
            }
        }
    }
}
