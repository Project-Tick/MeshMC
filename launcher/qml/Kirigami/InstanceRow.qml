// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.delegates as Delegates

/*
 * One instance in the library's list: a row that opens the instance's page
 * when clicked and its menu when right-clicked.
 */
Delegates.RoundedItemDelegate {
    id: row

    required property var app
    // The delegate's model object (see InstanceSummary).
    required property var model
    required property int index

    text: row.model.name
    Accessible.description: summary.versionLine
    onClicked: row.app.openInstance(row.model.instanceId)

    contentItem: InstanceSummary {
        id: summary
        app: row.app
        entry: row.model
        iconSize: Kirigami.Units.iconSizes.medium
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (eventPoint) => summary.openMenu(row, eventPoint.position)
    }
}
