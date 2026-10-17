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
 * Home's "Continue Playing": the instance played last, big -- its cover
 * beside its name, loader and version, when it was last played and for how
 * long in all, with Play first among its buttons. Side by side on a wide
 * page, the cover on top on a narrow one. A click anywhere else opens the
 * instance's page.
 *
 * `entry` is a row of the shell's recentModel (InstanceList's roles).
 */
Kirigami.AbstractCard {
    id: hero

    required property var app
    required property var entry

    readonly property var art: Artwork.cover(hero.entry)
    readonly property bool running: hero.entry.isRunning === true
    readonly property bool launching: (hero.entry.launchStatus || "").length > 0
    readonly property bool busy: hero.running || hero.launching
    readonly property real playedSeconds: Number(hero.entry.totalTimePlayed) || 0
    readonly property bool wide: hero.width > Kirigami.Units.gridUnit * 30

    showClickFeedback: true
    Accessible.name: hero.entry.name
    onClicked: hero.app.openInstance(hero.entry.instanceId)

    contentItem: GridLayout {
        columns: hero.wide ? 2 : 1
        columnSpacing: Kirigami.Units.largeSpacing * 2
        rowSpacing: Kirigami.Units.largeSpacing

        // The cover, on a plate in the view's alternate colour for a pack
        // logo shown whole.
        Rectangle {
            readonly property real coverWidth: hero.wide ? Math.min(Kirigami.Units.gridUnit * 22, parent.width * 0.45)
                                                         : parent.width
            Layout.preferredWidth: coverWidth
            Layout.preferredHeight: Math.round(coverWidth * 9 / 16)
            Layout.alignment: Qt.AlignTop
            color: Kirigami.Theme.alternateBackgroundColor
            Image {
                anchors.fill: parent
                source: hero.art.source
                sourceSize: Qt.size(Kirigami.Units.gridUnit * 32, Kirigami.Units.gridUnit * 18)
                fillMode: hero.art.logo ? Image.PreserveAspectFit : Image.PreserveAspectCrop
                asynchronous: true
                smooth: true
                mipmap: true
                clip: true
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: Kirigami.Units.smallSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                Kirigami.Icon {
                    implicitWidth: Kirigami.Units.iconSizes.large
                    implicitHeight: Kirigami.Units.iconSizes.large
                    source: "image://instanceicon/" + (hero.entry.iconKey && hero.entry.iconKey.length > 0
                                                       ? hero.entry.iconKey : "default")
                    // Instance icons are mostly pixel art.
                    smooth: false
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 2
                        text: hero.entry.name
                        elide: Text.ElideRight
                    }
                    LoaderLine {
                        Layout.fillWidth: true
                        loader: hero.entry.loader || ""
                        gameVersion: hero.entry.gameVersion || ""
                        logos: hero.app.shell ? hero.app.shell.brandLogos : null
                    }
                }
            }

            LastPlayedLabel {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.smallSpacing
                running: hero.running
                crashed: hero.entry.hasCrashed === true
                lastLaunch: hero.entry.lastLaunch
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: hero.playedSeconds >= 60
                text: i18nc("@info %1 is a duration such as '12.5 hours'", "Played for %1",
                            KCoreAddons.Format.formatDecimalDuration(hero.playedSeconds * 1000, 1))
                elide: Text.ElideRight
                opacity: 0.7
            }
            LaunchProgressBar {
                Layout.fillWidth: true
                visible: hero.launching
                progress: hero.entry.launchProgress
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: hero.launching
                text: hero.entry.launchStatus || ""
                elide: Text.ElideRight
                opacity: 0.7
            }

            RowLayout {
                Layout.topMargin: Kirigami.Units.largeSpacing
                spacing: Kirigami.Units.smallSpacing
                QQC2.Button {
                    text: hero.busy ? i18nc("@action:button", "Stop") : i18nc("@action:button", "Play")
                    icon.name: hero.busy ? "media-playback-stop" : "media-playback-start"
                    enabled: hero.busy || hero.entry.canLaunch !== false
                    highlighted: !hero.busy
                    onClicked: hero.busy ? hero.app.stop(hero.entry.instanceId) : hero.app.launch(hero.entry.instanceId)
                }
                QQC2.Button {
                    text: i18nc("@action:button", "Open")
                    icon.name: "go-next"
                    onClicked: hero.app.openInstance(hero.entry.instanceId)
                }
            }
        }
    }
}
