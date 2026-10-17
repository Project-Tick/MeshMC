// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls.impl
import org.kde.kirigami as Kirigami
import MeshMC.Theme

/*
 * A themed line icon: wraps Kirigami.Icon so callers pick an icon by name
 * from icons/ instead of building a source url and repeating the recolour
 * boilerplate. The SVGs are pure #000000 on transparent, so drawn as a mask
 * -- the way Kirigami recolours a monochrome theme icon -- every opaque
 * pixel takes `color`, and any palette colour tints them.
 */
Kirigami.Icon {
    id: root

    // `iconName`, not `name`, as when this was an IconImage (whose own
    // `name` is a theme icon name); every caller already uses it.
    property string iconName
    property int size: Theme.icon.md

    readonly property url iconUrl: iconName.length > 0 ? Qt.resolvedUrl("icons/" + iconName + ".svg") : ""
    // Kirigami recolours a mask in a shader, which the software renderer
    // cannot run -- it would draw the SVG's own black. There (and in the
    // offscreen review snapshots, which use it) IconImage's CPU recolouring,
    // what this used to be, draws the icon instead; see the same gate in
    // SkinPreview.qml.
    readonly property bool maskInShader: GraphicsInfo.api !== GraphicsInfo.Software

    width: size
    height: size
    implicitWidth: size
    implicitHeight: size
    source: root.maskInShader ? root.iconUrl : ""
    isMask: true
    color: Theme.palette.textSecondary
    // Drawn at exactly `size`: Kirigami would otherwise snap it to the
    // nearest standard icon size (16, 22, 32...), and 20 is not one.
    roundToIconSize: false
    // An empty or missing name draws nothing, not the theme's "unknown".
    fallback: ""

    IconImage {
        anchors.fill: parent
        visible: !root.maskInShader
        source: visible ? root.iconUrl : ""
        sourceSize: Qt.size(root.size, root.size)
        color: root.color
    }
}
