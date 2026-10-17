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
 * The head of an instance's page, as Discover heads an application's: the
 * picture its library card shows (artwork.js: its pack's art, else its
 * newest screenshot, else one of the launcher's scenes) as a wide, short
 * banner, darkened towards its foot, and over it the instance's icon, its
 * name, its loader and version, and what there is to know at a glance --
 * when it was last played, for how long in all, how many mods it has.
 *
 * The text is in the Complementary colours, light on dark, as KDE puts
 * text over pictures: it reads the same on any picture and in any scheme.
 * A pack's square logo is not cut into a strip like a picture: it sits
 * whole at the banner's far end.
 *
 * `entry` is the instance's row of the shell's instancePageModel
 * (InstanceList's roles), null until there is one; `content` its
 * InstanceContentController (InstanceDetails.content), null for an
 * old-format instance, which has no content to count.
 */
Item {
    id: banner

    property var entry: null
    // The name to show before `entry` is there.
    property string title
    property var content: null
    // The shell's BrandLogos (see BrandIcon).
    property var logos: null

    readonly property var art: banner.entry ? Artwork.cover(banner.entry) : null
    readonly property real playedSeconds: banner.entry ? Number(banner.entry.totalTimePlayed) || 0 : 0
    // The mods folder's list -- the very one the Content tab shows, so
    // asking for it here costs nothing the page would not spend anyway.
    readonly property var mods: {
        if (!banner.content || banner.content.kinds.indexOf("mods") < 0)
            return null
        return banner.content.model("mods")
    }
    readonly property int modCount: banner.mods ? banner.mods.totalCount : 0
    // "0 mods" says nothing about an instance without a mod loader.
    readonly property bool showMods: !!banner.mods && (banner.content.hasModLoader || banner.modCount > 0)
    // After when it was last played.
    readonly property var facts: {
        var list = []
        if (banner.playedSeconds >= 60)
            list.push(i18nc("@info %1 is a duration such as '12.5 hours'", "Played for %1",
                            KCoreAddons.Format.formatDecimalDuration(banner.playedSeconds * 1000, 1)))
        if (banner.showMods)
            list.push(i18ncp("@info", "%1 mod", "%1 mods", banner.modCount))
        return list
    }

    implicitHeight: Kirigami.Units.gridUnit * 10
    clip: true

    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
    Kirigami.Theme.inherit: false

    Rectangle {
        anchors.fill: parent
        color: Kirigami.Theme.backgroundColor
    }
    // Cropped to the banner, or a logo whole at its end. Decoded at most
    // this wide: a banner is rarely wider, and a screenshot can be 4K.
    Image {
        readonly property bool logo: !!banner.art && banner.art.logo
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: logo ? Kirigami.Units.gridUnit : 0
        width: logo ? height : parent.width
        source: banner.art ? banner.art.source : ""
        sourceSize.width: Kirigami.Units.gridUnit * 80
        fillMode: logo ? Image.PreserveAspectFit : Image.PreserveAspectCrop
        asynchronous: true
        smooth: true
        mipmap: true
    }
    // Darker towards the foot, where the text is. A logo leaves the text
    // on the plain background, which needs none.
    Rectangle {
        id: shade
        readonly property color base: Kirigami.Theme.backgroundColor
        anchors.fill: parent
        visible: !!banner.art && !banner.art.logo
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(shade.base.r, shade.base.g, shade.base.b, 0.05) }
            GradientStop { position: 0.35; color: Qt.rgba(shade.base.r, shade.base.g, shade.base.b, 0.3) }
            GradientStop { position: 1.0; color: Qt.rgba(shade.base.r, shade.base.g, shade.base.b, 0.92) }
        }
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Kirigami.Units.gridUnit
        spacing: Kirigami.Units.largeSpacing * 2

        Kirigami.Icon {
            Layout.alignment: Qt.AlignBottom
            implicitWidth: Kirigami.Units.iconSizes.huge
            implicitHeight: Kirigami.Units.iconSizes.huge
            source: "image://instanceicon/" + (banner.entry && banner.entry.iconKey && banner.entry.iconKey.length > 0
                                               ? banner.entry.iconKey : "default")
            // Instance icons are mostly pixel art.
            smooth: false
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                Layout.fillWidth: true
                level: 1
                text: banner.entry ? banner.entry.name : banner.title
                elide: Text.ElideRight
            }
            LoaderLine {
                Layout.fillWidth: true
                visible: !!banner.entry
                loader: banner.entry ? banner.entry.loader || "" : ""
                gameVersion: banner.entry ? banner.entry.gameVersion || "" : ""
                logos: banner.logos
            }
            RowLayout {
                Layout.fillWidth: true
                visible: !!banner.entry
                spacing: 0

                LastPlayedLabel {
                    Layout.maximumWidth: implicitWidth
                    Layout.fillWidth: true
                    running: !!banner.entry && banner.entry.isRunning === true
                    crashed: !!banner.entry && banner.entry.hasCrashed === true
                    lastLaunch: banner.entry ? banner.entry.lastLaunch : 0
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    visible: banner.facts.length > 0
                    text: [""].concat(banner.facts).join(" · ")
                    elide: Text.ElideRight
                    opacity: 0.7
                }
            }
        }
    }
}
