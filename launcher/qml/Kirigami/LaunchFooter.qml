// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * The instance page's footer while the game starts: what the launch is
 * doing, how far along it is, and Cancel. Hidden the rest of the time --
 * once the game runs, the toolbar's Stop takes over.
 */
QQC2.ToolBar {
    id: footer

    // The launch task's current step; empty when nothing is starting.
    property string status
    // 0..1, or below 0 when the step cannot say how far it is.
    property real progress: -1

    signal cancelRequested()

    visible: footer.status.length > 0

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.maximumWidth: Kirigami.Units.gridUnit * 16
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
        QQC2.Button {
            text: i18nc("@action:button", "Cancel")
            icon.name: "dialog-cancel"
            onClicked: footer.cancelRequested()
        }
    }
}
