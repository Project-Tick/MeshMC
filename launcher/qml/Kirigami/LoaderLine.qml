// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import "brands.js" as Brands

/*
 * What an instance runs: its mod loader's logo, then "Fabric 1.21.4" --
 * or the grass block and "1.21.4" for vanilla, which has no loader.
 * `loader` and `gameVersion` are InstanceList's roles of the same names.
 */
RowLayout {
    id: line

    property string loader
    property string gameVersion
    // The shell's BrandLogos (see BrandIcon).
    property var logos: null
    property int iconSize: Kirigami.Units.iconSizes.small
    // A status to show in place of the version ("Running · ..."), if any.
    property string text: line.versionText

    readonly property string versionText: {
        var parts = [line.loader, line.gameVersion].filter(s => s && s.length > 0)
        return parts.length > 0 ? parts.join(" ") : i18n("Minecraft")
    }

    spacing: Kirigami.Units.smallSpacing

    BrandIcon {
        Layout.alignment: Qt.AlignVCenter
        brand: Brands.loaderId(line.loader)
        logos: line.logos
        size: line.iconSize
        fallback: "applications-games-symbolic"
    }
    QQC2.Label {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
        text: line.text
        elide: Text.ElideRight
        opacity: 0.7
    }
}
