// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * A page's footer while something it started downloads -- a modpack, a
 * mod, updates: what it is doing, how far along it is, and the percentage
 * when there is one. LaunchFooter is the same for a game starting, with
 * Cancel.
 */
QQC2.ToolBar {
    id: footer

    // What it is doing now; the bar shows while `active`.
    property string status
    // 0..1, or below 0 when it cannot say how far it is.
    property real progress: -1
    property bool active: false

    visible: footer.active
    position: QQC2.ToolBar.Footer

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.maximumWidth: Kirigami.Units.gridUnit * 18
            text: footer.status
            elide: Text.ElideRight
        }
        LaunchProgressBar {
            Layout.fillWidth: true
            progress: footer.progress
            Accessible.name: footer.status
        }
        QQC2.Label {
            visible: footer.progress >= 0
            text: i18nc("@info:progress", "%1%", Math.round(footer.progress * 100))
        }
    }
}
