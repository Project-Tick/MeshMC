// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * A skin on a player, as large as the box it is given: in 3D, turned by
 * dragging, where the build has Qt Quick3D (`preview3DAvailable`,
 * MESHMC_HAS_SKIN_PREVIEW_3D) and the window draws through a 3D graphics
 * API -- the model is MeshMC.Preview3D's SkinPreview3D.qml, loaded by url so
 * this module needs no Quick3D of its own -- and otherwise the flat front
 * view (`flatSource`, image://accountface/body/<id> and friends). Qt
 * Quick3D draws nothing through the software renderer, which the offscreen
 * review snapshots use, so they show the flat view.
 *
 * On the View colours, as a document area, so a dark skin still stands
 * off its background.
 */
Rectangle {
    id: preview

    // The 64x64 skin texture; empty when there is no skin to show.
    property url skinTexture
    property url flatSource
    property bool slim: false
    property url capeTexture
    property bool elytra: false
    property bool preview3DAvailable: false

    readonly property bool hasSkin: preview.skinTexture.toString().length > 0
    readonly property bool threeD: preview.preview3DAvailable && preview.hasSkin
                                   && GraphicsInfo.api !== GraphicsInfo.Software
                                   && GraphicsInfo.api !== GraphicsInfo.Unknown
                                   && model.status !== Loader.Error

    function showBack() {
        if (model.item)
            model.item.showBack()
    }
    function showFront() {
        if (model.item)
            model.item.showFront()
    }

    Kirigami.Theme.colorSet: Kirigami.Theme.View
    Kirigami.Theme.inherit: false
    color: Kirigami.Theme.backgroundColor
    radius: Kirigami.Units.cornerRadius

    Loader {
        id: model
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        active: preview.preview3DAvailable && preview.hasSkin
                && GraphicsInfo.api !== GraphicsInfo.Software && GraphicsInfo.api !== GraphicsInfo.Unknown
        source: active ? "qrc:/qt/qml/MeshMC/Preview3D/SkinPreview3D.qml" : ""
        onLoaded: {
            item.skinTexture = Qt.binding(() => preview.skinTexture)
            item.slim = Qt.binding(() => preview.slim)
            item.capeTexture = Qt.binding(() => preview.capeTexture)
            item.elytra = Qt.binding(() => preview.elytra)
            // An elytra without a cape is plain.
            item.plainColor = Qt.binding(() => Kirigami.Theme.disabledTextColor)
            item.turnDuration = Kirigami.Units.longDuration
            item.shadowColor = Qt.binding(() => Qt.alpha(Kirigami.Theme.textColor, 0.35))
        }
    }

    // The flat view: a 1:2 figure, centred, as large as fits.
    Image {
        id: flat
        visible: !preview.threeD && preview.hasSkin && status === Image.Ready
        anchors.centerIn: parent
        readonly property real figureHeight: Math.min(parent.height, parent.width * 2) - Kirigami.Units.largeSpacing * 2
        width: figureHeight / 2
        height: figureHeight
        source: preview.hasSkin ? preview.flatSource : ""
        sourceSize: Qt.size(width * 2, height * 2)
        fillMode: Image.PreserveAspectFit
        smooth: false
    }

    // Nothing to draw yet: an account without a skin of its own wears the
    // game's default once that has downloaded.
    Kirigami.Icon {
        anchors.centerIn: parent
        visible: !preview.threeD && !flat.visible
        source: "user-identity"
        width: Kirigami.Units.iconSizes.huge
        height: width
        color: Kirigami.Theme.disabledTextColor
    }
}
