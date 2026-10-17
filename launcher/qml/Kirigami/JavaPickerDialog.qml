// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

/*
 * One of the Java installs MeshMC found on this computer, the runtimes it
 * downloaded itself among them. Picking a row emits picked() and closes;
 * the caller decides what to do with the path.
 */
Kirigami.Dialog {
    id: dialog

    // JavaController: installs, detecting, detect().
    property var java: null
    // The path in use now, marked in the list.
    property string currentPath

    signal picked(string path, string version)

    title: i18nc("@title:dialog", "Choose Java")
    preferredWidth: Kirigami.Units.gridUnit * 30
    preferredHeight: Kirigami.Units.gridUnit * 22
    standardButtons: Kirigami.Dialog.Close
    customFooterActions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Detect Again")
            icon.name: "view-refresh"
            enabled: !!dialog.java && !dialog.java.detecting
            onTriggered: dialog.java.detect()
        }
    ]

    onOpened: if (dialog.java && list.count === 0 && !dialog.java.detecting) dialog.java.detect()

    ListView {
        id: list
        clip: true
        model: dialog.java ? dialog.java.installs : null

        delegate: QQC2.ItemDelegate {
            id: row
            required property string path
            required property var version
            required property bool recommended

            width: ListView.view.width
            highlighted: dialog.currentPath === row.path
            text: i18nc("@item:inlistbox", "Java %1", String(row.version))
            onClicked: {
                dialog.picked(row.path, String(row.version))
                dialog.close()
            }
            contentItem: KD.IconTitleSubtitle {
                title: row.text
                subtitle: row.recommended ? i18nc("@info %1 is a path", "Recommended · %1", row.path) : row.path
                icon.name: row.recommended ? "starred-symbolic" : "application-x-java"
                selected: row.highlighted
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: list.count === 0
            icon.name: "application-x-java"
            text: dialog.java && dialog.java.detecting ? i18n("Looking for Java…") : i18n("No Java found")
            explanation: dialog.java && dialog.java.detecting ? ""
                         : i18n("Download one in Settings, or browse for a Java you installed yourself.")
        }
    }
}
