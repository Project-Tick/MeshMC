// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import "artwork.js" as Artwork

/*
 * One instance in the library's grid: a card with the instance's cover on
 * top (artwork.js: its pack's art, else its newest screenshot, else its own
 * icon), then its name, its loader's logo and version and
 * when it was last played, and Play with the instance's menu at the foot.
 * A click opens the instance's page; a right-click, its menu.
 *
 * `model` is the delegate's model object, with InstanceList's roles
 * (instanceId, name, iconKey, group, isRunning, canLaunch, gameVersion,
 * loader, launchStatus, launchProgress, lastLaunch, coverImage, packArt,
 * hasCrashed).
 */
Kirigami.Card {
    id: card

    required property var app
    required property var model

    readonly property var art: Artwork.cover(card.model)
    readonly property bool running: card.model.isRunning === true
    readonly property bool launching: (card.model.launchStatus || "").length > 0
    readonly property bool busy: card.running || card.launching
    // What InstanceDialogs.openMenu() needs to build the menu.
    readonly property var info: ({
        instanceId: card.model.instanceId,
        name: card.model.name,
        iconKey: card.model.iconKey,
        group: card.model.group,
        running: card.busy,
        canLaunch: card.model.canLaunch !== false
    })

    function openMenu(parent, position) {
        card.app.dialogs.openMenu(card.info, parent, position)
    }

    showClickFeedback: true
    Accessible.name: card.model.name
    Accessible.description: loaderLine.versionText
    onClicked: card.app.openInstance(card.model.instanceId)

    // Every cover is decoded at one 16:9 size, so every banner has the
    // same shape: a picture is cropped to it, a pack's square logo or the
    // instance's icon sits whole in the middle of it.
    banner.source: card.art.source
    banner.sourceSize: Qt.size(Kirigami.Units.gridUnit * 32, Kirigami.Units.gridUnit * 18)
    banner.fillMode: card.art.logo ? Image.PreserveAspectFit : Image.PreserveAspectCrop
    banner.color: Kirigami.Theme.alternateBackgroundColor
    banner.asynchronous: true

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 4
            text: card.model.name
            elide: Text.ElideRight
        }
        LoaderLine {
            id: loaderLine
            Layout.fillWidth: true
            loader: card.model.loader || ""
            gameVersion: card.model.gameVersion || ""
            logos: card.app.shell ? card.app.shell.brandLogos : null
        }
        LastPlayedLabel {
            Layout.fillWidth: true
            visible: !card.launching
            running: card.running
            crashed: card.model.hasCrashed === true
            lastLaunch: card.model.lastLaunch
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: card.launching
            text: card.model.launchStatus || ""
            elide: Text.ElideRight
            opacity: 0.7
        }
        LaunchProgressBar {
            Layout.fillWidth: true
            visible: card.launching
            progress: card.model.launchProgress
        }
    }

    actions: [
        Kirigami.Action {
            text: card.busy ? i18nc("@action:button", "Stop") : i18nc("@action:button", "Play")
            icon.name: card.busy ? "media-playback-stop" : "media-playback-start"
            enabled: card.busy || card.info.canLaunch
            onTriggered: card.busy ? card.app.stop(card.model.instanceId) : card.app.launch(card.model.instanceId)
        },
        Kirigami.Action {
            text: i18nc("@action:button", "More Actions")
            icon.name: "overflow-menu"
            displayHint: Kirigami.DisplayHint.IconOnly
            onTriggered: card.openMenu(card, Qt.point(card.width - Kirigami.Units.gridUnit, card.height))
        }
    ]

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (eventPoint) => card.openMenu(card, eventPoint.position)
    }
}
