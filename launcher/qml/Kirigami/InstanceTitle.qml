// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * An instance page's title in the toolbar: the instance's own icon, its
 * name, and its loader's logo with the loader and Minecraft version -- what
 * the library's card says of it, so the page reads as that instance.
 *
 * `entry` is the instance's row of the shell's instancePageModel
 * (InstanceList's roles), or null while there is none.
 */
RowLayout {
    id: titleRow

    property var entry: null
    property string title
    // The shell's BrandLogos (see BrandIcon).
    property var logos: null

    // As wide as it needs and no wider: a layout fills by default, and the
    // page header then splits its width between this and the actions, which
    // pushed Play into the overflow menu at body-text sizes (13pt on macOS).
    // Short of room it still shrinks, eliding the name.
    Layout.fillWidth: false
    spacing: Kirigami.Units.largeSpacing

    Kirigami.Icon {
        Layout.alignment: Qt.AlignVCenter
        visible: !!titleRow.entry
        implicitWidth: Kirigami.Units.iconSizes.smallMedium
        implicitHeight: Kirigami.Units.iconSizes.smallMedium
        source: titleRow.entry ? "image://instanceicon/" + (titleRow.entry.iconKey && titleRow.entry.iconKey.length > 0
                                                         ? titleRow.entry.iconKey : "default") : ""
        // Instance icons are mostly pixel art.
        smooth: false
    }
    Kirigami.Heading {
        Layout.alignment: Qt.AlignVCenter
        Layout.minimumWidth: 0
        Layout.maximumWidth: Math.ceil(implicitWidth)
        Layout.fillWidth: true
        level: 1
        text: titleRow.title
        elide: Text.ElideRight
    }
    LoaderLine {
        Layout.alignment: Qt.AlignVCenter
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        visible: !!titleRow.entry
        loader: titleRow.entry ? titleRow.entry.loader || "" : ""
        gameVersion: titleRow.entry ? titleRow.entry.gameVersion || "" : ""
        logos: titleRow.logos
    }
}
