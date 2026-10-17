// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * One instance in the library's grid: a card that opens the instance's page
 * when clicked and its menu when right-clicked.
 */
Kirigami.AbstractCard {
    id: card

    required property var app
    // The delegate's model object (see InstanceSummary).
    required property var model

    showClickFeedback: true
    Accessible.name: card.model.name
    Accessible.description: summary.versionLine
    onClicked: card.app.openInstance(card.model.instanceId)

    contentItem: InstanceSummary {
        id: summary
        app: card.app
        entry: card.model
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (eventPoint) => summary.openMenu(card, eventPoint.position)
    }
}
