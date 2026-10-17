// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * A modpack's or mod's own logo, from its site; one of the launcher's own
 * block pictures (`fallbackSource`) when it has none or it cannot be
 * loaded, as the game itself shows a pack without a picture as a block.
 * Nothing while the logo is on its way, rather than a stand-in swapped a
 * moment later.
 *
 * Images rather than a Kirigami.Icon: the logos come from the network,
 * often as WebP, and an Image says reliably whether one has loaded (an Icon
 * can report an error for a logo it then shows).
 */
Item {
    id: logo

    // The project's logo URL, or "".
    property string logoUrl
    property string fallbackSource: "image://instanceicon/planks"

    readonly property bool showsLogo: remote.status === Image.Ready
    readonly property bool failed: logo.logoUrl.length === 0 || remote.status === Image.Error
    // What is shown has finished loading (for ImageColors).
    signal loaded()

    Image {
        id: remote
        anchors.fill: parent
        source: logo.logoUrl
        sourceSize.width: logo.width * 2
        sourceSize.height: logo.height * 2
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        smooth: true
        mipmap: true
        visible: logo.showsLogo
        onStatusChanged: if (status === Image.Ready) logo.loaded()
    }
    Image {
        anchors.fill: parent
        visible: logo.failed
        source: logo.failed ? logo.fallbackSource : ""
        sourceSize.width: logo.width * 2
        sourceSize.height: logo.height * 2
        fillMode: Image.PreserveAspectFit
        // The launcher's block pictures are pixel art.
        smooth: false
        onStatusChanged: if (status === Image.Ready) logo.loaded()
    }
}
