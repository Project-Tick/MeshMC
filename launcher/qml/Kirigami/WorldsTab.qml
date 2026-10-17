// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.coreaddons as KCoreAddons
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD
import org.kde.kirigamiaddons.delegates as Delegates

/*
 * The instance's worlds: each with its icon, name, game mode and when it
 * was last played. Copying, renaming, backups and data packs come with the
 * rest of the instance page in a later phase; for now the folder opens.
 */
Item {
    id: tab

    required property var app
    property var details: null

    readonly property var worlds: tab.details ? tab.details.worlds : null

    // A world's icon file as a URL Image can load; "" when it has none.
    function fileUrl(path) {
        if (!path || String(path).length === 0)
            return ""
        var p = String(path).replace(/\\/g, "/")
        return "file://" + (p.charAt(0) === "/" ? "" : "/") + p
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        QQC2.ToolBar {
            Layout.fillWidth: true
            contentItem: Kirigami.ActionToolBar {
                alignment: Qt.AlignRight
                actions: [
                    Kirigami.Action {
                        text: i18nc("@action:button", "Refresh")
                        icon.name: "view-refresh"
                        onTriggered: tab.details.refreshWorlds()
                    },
                    Kirigami.Action {
                        text: i18nc("@action:button", "Open Folder")
                        icon.name: "document-open-folder"
                        onTriggered: tab.app.call("openPath", tab.details.worldsDir)
                    }
                ]
            }
        }

        ListArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                clip: true
                model: tab.worlds
                delegate: Delegates.RoundedItemDelegate {
                    id: row
                    required property int index
                    required property string name
                    required property string folder
                    required property var gameMode
                    required property var lastPlayed
                    required property var iconFile

                    width: ListView.view.width
                    text: row.name
                    contentItem: KD.IconTitleSubtitle {
                        title: row.name
                        subtitle: {
                            var parts = []
                            if (row.gameMode && String(row.gameMode).length > 0)
                                parts.push(String(row.gameMode))
                            if (row.lastPlayed)
                                parts.push(i18nc("@info %1 is a date", "Played %1",
                                                 KCoreAddons.Format.formatRelativeDateTime(row.lastPlayed, Locale.ShortFormat)))
                            return parts.join(" · ")
                        }
                        selected: row.highlighted
                        icon.source: tab.fileUrl(row.iconFile)
                        icon.name: tab.fileUrl(row.iconFile).length > 0 ? "" : "map-globe"
                        icon.width: Kirigami.Units.iconSizes.medium
                        icon.height: Kirigami.Units.iconSizes.medium
                    }
                    onDoubleClicked: tab.app.call("openPath", tab.details.worldsDir + "/" + row.folder)
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: list.count === 0
                    icon.name: "map-globe"
                    text: i18n("No worlds yet")
                    explanation: i18n("Worlds you create in the game show up here.")
                }
            }
        }
    }
}
