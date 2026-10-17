// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

/*
 * A project's pictures as one row of thumbnails, scrolled sideways; a click
 * opens one full size (GalleryViewer). `images` is a gallery as the
 * provider models give it: [{url, title, featured}].
 */
ListView {
    id: strip

    property var images: []
    signal opened(int index)

    readonly property int thumbHeight: Kirigami.Units.gridUnit * 7
    // Screenshots are 16:9 more often than not; anything else is cropped.
    readonly property int thumbWidth: Math.round(strip.thumbHeight * 16 / 9)

    implicitHeight: strip.thumbHeight + (scrollBar.visible ? scrollBar.height + Kirigami.Units.smallSpacing : 0)
    orientation: ListView.Horizontal
    spacing: Kirigami.Units.largeSpacing
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    model: strip.images
    QQC2.ScrollBar.horizontal: QQC2.ScrollBar {
        id: scrollBar
    }

    delegate: QQC2.AbstractButton {
        id: thumb
        required property var modelData
        required property int index
        width: strip.thumbWidth
        height: strip.thumbHeight
        hoverEnabled: true
        text: thumb.modelData.title || i18nc("@info:tooltip %1 is a number", "Picture %1", thumb.index + 1)
        Accessible.name: thumb.text
        onClicked: strip.opened(thumb.index)

        // A frame in the view colours, the picture inside it; the frame
        // takes the focus colour on hover, as a thumbnail view's does.
        background: Rectangle {
            color: Kirigami.Theme.alternateBackgroundColor
            radius: Kirigami.Units.cornerRadius
            border.width: 1
            border.color: thumb.hovered || thumb.visualFocus ? Kirigami.Theme.focusColor
                        : Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g,
                                  Kirigami.Theme.textColor.b, Kirigami.Theme.frameContrast)
        }
        padding: 1
        contentItem: Image {
            source: thumb.modelData.url || ""
            sourceSize.width: thumb.width * 2
            sourceSize.height: thumb.height * 2
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            smooth: true
            mipmap: true
            clip: true
        }

        QQC2.ToolTip.text: thumb.modelData.title || ""
        QQC2.ToolTip.visible: thumb.hovered && (thumb.modelData.title || "").length > 0
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    }
}
