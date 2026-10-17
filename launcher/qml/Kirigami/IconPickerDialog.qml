// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import org.kde.kirigami as Kirigami

/*
 * "Change Icon…": every icon the launcher has (the built-in ones and those
 * in its icons folder, QmlShell's iconsModel), one to pick. "Add Icon…"
 * copies an image file into the icons folder and picks it.
 */
Kirigami.Dialog {
    id: dialog

    required property var shell
    // The key picked, or the instance's own when the dialog opens.
    property string current

    signal picked(string key)

    readonly property int cellSize: Kirigami.Units.iconSizes.huge + Kirigami.Units.largeSpacing * 2

    title: i18nc("@title:dialog", "Change Icon")
    padding: Kirigami.Units.largeSpacing
    preferredWidth: Kirigami.Units.gridUnit * 30
    preferredHeight: Kirigami.Units.gridUnit * 24
    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    onAccepted: if (dialog.current.length > 0) dialog.picked(dialog.current)

    customFooterActions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Add Icon…")
            icon.name: "list-add"
            onTriggered: iconFile.open()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Open Folder")
            icon.name: "document-open-folder"
            onTriggered: dialog.shell.openPath(dialog.shell.iconsDir)
        }
    ]

    GridView {
        id: grid
        clip: true
        model: dialog.shell ? dialog.shell.iconsModel : null
        cellWidth: dialog.cellSize
        cellHeight: dialog.cellSize
        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}

        delegate: QQC2.ItemDelegate {
            id: cell
            required property string key
            required property string name
            width: grid.cellWidth
            height: grid.cellHeight
            highlighted: dialog.current === cell.key
            Accessible.name: cell.name
            QQC2.ToolTip.text: cell.name
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            contentItem: Image {
                source: "image://instanceicon/" + cell.key
                sourceSize.width: Kirigami.Units.iconSizes.huge
                sourceSize.height: Kirigami.Units.iconSizes.huge
                fillMode: Image.PreserveAspectFit
                smooth: false
            }
            onClicked: dialog.current = cell.key
            onDoubleClicked: {
                dialog.current = cell.key
                dialog.accept()
            }
        }
    }

    Dialogs.FileDialog {
        id: iconFile
        title: i18nc("@title:dialog", "Add Icon")
        nameFilters: [i18n("Images (*.png *.jpg *.jpeg *.gif *.svg *.ico *.icns)")]
        onAccepted: dialog.shell.importIcon(selectedFile.toString())
    }
    // The copy lands in the icons folder under a key of its own: pick it.
    Connections {
        target: dialog.shell
        ignoreUnknownSignals: true
        function onIconImported(key) {
            dialog.current = key
        }
    }
}
