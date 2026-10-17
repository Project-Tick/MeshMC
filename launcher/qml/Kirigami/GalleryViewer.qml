// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * A project's pictures full size, over the whole window, one at a time:
 * arrows (or the arrow keys) between them, its title underneath, Escape or
 * a click beside the picture to close. In the Complementary colours, as
 * KDE's image viewers show media.
 *
 * Kirigami Addons' AlbumMaximizeComponent does the same, but reads its
 * current item before the item exists and warns each time it is made.
 *
 * `images` is a provider gallery: [{url, title, featured}].
 */
QQC2.Popup {
    id: viewer

    property var images: []
    property int index: 0
    readonly property var current: viewer.images.length > 0 ? viewer.images[viewer.index] : null

    function show(index) {
        viewer.index = Math.max(0, Math.min(index, viewer.images.length - 1))
        viewer.open()
    }
    function step(by) {
        var next = viewer.index + by
        if (next >= 0 && next < viewer.images.length)
            viewer.index = next
    }

    parent: QQC2.Overlay.overlay
    x: 0
    y: 0
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    modal: true
    focus: true
    padding: 0
    closePolicy: QQC2.Popup.CloseOnEscape

    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
    Kirigami.Theme.inherit: false

    background: Rectangle {
        Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
        Kirigami.Theme.inherit: false
        color: Kirigami.Theme.backgroundColor
        opacity: 0.95
    }

    contentItem: Item {
        Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
        Kirigami.Theme.inherit: false
        focus: true
        Keys.onLeftPressed: viewer.step(-1)
        Keys.onRightPressed: viewer.step(1)

        // A click beside the picture closes it.
        TapHandler {
            onTapped: viewer.close()
        }

        Image {
            id: picture
            anchors.fill: parent
            anchors.margins: Kirigami.Units.gridUnit * 3
            source: viewer.opened && viewer.current ? (viewer.current.url || "") : ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            smooth: true
            mipmap: true
            // The picture's own clicks are not "beside" it.
            TapHandler {}
        }
        QQC2.BusyIndicator {
            anchors.centerIn: parent
            running: picture.status === Image.Loading
            visible: running
        }
        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 8
            visible: picture.status === Image.Error
            icon.name: "image-missing-symbolic"
            text: i18n("This picture could not be loaded")
        }

        QQC2.RoundButton {
            anchors.left: parent.left
            anchors.leftMargin: Kirigami.Units.largeSpacing
            anchors.verticalCenter: parent.verticalCenter
            visible: viewer.index > 0
            icon.name: "go-previous"
            text: i18nc("@action:button", "Previous Picture")
            display: QQC2.AbstractButton.IconOnly
            onClicked: viewer.step(-1)
        }
        QQC2.RoundButton {
            anchors.right: parent.right
            anchors.rightMargin: Kirigami.Units.largeSpacing
            anchors.verticalCenter: parent.verticalCenter
            visible: viewer.index < viewer.images.length - 1
            icon.name: "go-next"
            text: i18nc("@action:button", "Next Picture")
            display: QQC2.AbstractButton.IconOnly
            onClicked: viewer.step(1)
        }

        RowLayout {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing
            QQC2.Label {
                Layout.fillWidth: true
                text: viewer.images.length > 1
                      ? i18nc("@info %1 this picture, %2 how many there are", "%1 of %2", viewer.index + 1, viewer.images.length)
                      : ""
            }
            QQC2.ToolButton {
                icon.name: "dialog-close"
                text: i18nc("@action:button", "Close")
                display: QQC2.AbstractButton.IconOnly
                onClicked: viewer.close()
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }

        QQC2.Label {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: Kirigami.Units.largeSpacing
            width: Math.min(implicitWidth, parent.width - Kirigami.Units.gridUnit * 4)
            visible: text.length > 0
            text: viewer.current ? (viewer.current.title || "") : ""
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
