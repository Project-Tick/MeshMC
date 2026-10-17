// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

import "brands.js" as Brands

/*
 * One mod loader's or modpack site's logo (see brands.js), square, at
 * `size`. A logo still to be fetched, or that cannot be, shows `fallback`
 * from the icon theme instead. Decorative: the name beside it says what it
 * is, so it carries none of its own.
 */
Item {
    id: icon

    // "modrinth", "fabric", "vanilla", ...: see brands.js.
    property string brand
    // The shell's BrandLogos, for the logos fetched on first use.
    property var logos: null
    property int size: Kirigami.Units.iconSizes.small
    property string fallback: "globe-symbolic"

    readonly property string source: Brands.source(icon.brand, icon.logos)
    readonly property bool ready: image.status === Image.Ready

    implicitWidth: icon.size
    implicitHeight: icon.size
    Accessible.ignored: true

    Image {
        id: image
        anchors.fill: parent
        source: icon.source
        sourceSize.width: icon.width * 2
        sourceSize.height: icon.height * 2
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        smooth: !Brands.isPixelArt(icon.brand)
        mipmap: !Brands.isPixelArt(icon.brand)
        visible: icon.ready
    }
    Kirigami.Icon {
        anchors.fill: parent
        visible: !icon.ready && icon.fallback.length > 0
        source: icon.fallback
    }
}
