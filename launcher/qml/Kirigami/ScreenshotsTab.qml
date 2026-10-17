// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * The instance's screenshots, newest first, as thumbnails; one opens in the
 * system's image viewer. Renaming, deleting and uploading come with the rest
 * of the instance page in a later phase.
 */
Item {
    id: tab

    required property var app
    property var details: null

    readonly property var shots: tab.details ? tab.details.screenshots : null
    readonly property int thumbnailWidth: Kirigami.Units.gridUnit * 12
    // Thumbnails are cut to the shape of a typical game window.
    readonly property real thumbnailAspect: 16 / 9

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        QQC2.ToolBar {
            Layout.fillWidth: true
            contentItem: Kirigami.ActionToolBar {
                alignment: Qt.AlignRight
                actions: [
                    Kirigami.Action {
                        text: i18nc("@action:button", "Open Folder")
                        icon.name: "document-open-folder"
                        onTriggered: tab.app.call("openPath", tab.details.screenshotsDir)
                    }
                ]
            }
        }

        ListArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            GridView {
                id: grid
                clip: true
                model: tab.shots
                // Columns fill the width, one more once there is room for
                // another thumbnail.
                readonly property int columns: Math.max(1, Math.floor(width / tab.thumbnailWidth))
                cellWidth: Math.floor(width / columns)
                // The thumbnail, with a line for the name below.
                cellHeight: Math.round(cellWidth / tab.thumbnailAspect) + Kirigami.Units.gridUnit * 2

                delegate: QQC2.ItemDelegate {
                    id: cell
                    required property string name
                    required property string path
                    required property string url

                    width: grid.cellWidth
                    height: grid.cellHeight
                    Accessible.name: cell.name
                    onClicked: Qt.openUrlExternally(cell.url)

                    contentItem: ColumnLayout {
                        spacing: Kirigami.Units.smallSpacing
                        Image {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            source: "image://screenshot/" + encodeURIComponent(cell.path)
                            sourceSize.width: width
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            clip: true
                        }
                        QQC2.Label {
                            Layout.fillWidth: true
                            text: cell.name
                            elide: Text.ElideMiddle
                            horizontalAlignment: Text.AlignHCenter
                            font: Kirigami.Theme.smallFont
                        }
                    }
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: grid.count === 0
                    icon.name: "folder-pictures"
                    text: i18n("No screenshots yet")
                    explanation: i18n("Press F2 in the game to take one.")
                }
            }
        }
    }
}
