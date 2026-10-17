// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * A section whose page has not moved to the KDE interface yet: its title,
 * and a message saying it is on its way.
 */
Kirigami.Page {
    id: page

    property string iconName
    property string text

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        icon.name: page.iconName
        text: page.text
        explanation: i18n("This part of MeshMC is being rebuilt for KDE.")
    }
}
