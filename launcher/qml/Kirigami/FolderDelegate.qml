// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import "paths.js" as Paths

/*
 * One of MeshMC's folders: what it holds, where it is now, Change… to pick
 * another place and Open to show it in the file manager. The change goes
 * through FolderSettingsController, which asks about risky places (a "!"
 * in the path, a folder lent through a Flatpak portal) before saving. What
 * is already in the folder is not moved.
 */
FormCard.FormTextDelegate {
    id: delegate

    required property var source
    required property string key
    // FolderSettingsController, or null (then only Open is offered).
    property var folders: null
    readonly property string path: delegate.source.string(delegate.key)

    signal openRequested(string path)

    description: delegate.path
    trailing: RowLayout {
        spacing: Kirigami.Units.smallSpacing
        QQC2.Button {
            visible: !!delegate.folders
            text: i18nc("@action:button", "Change…")
            onClicked: dialog.open()
        }
        QQC2.Button {
            icon.name: "document-open-folder"
            text: i18nc("@action:button", "Open Folder")
            display: QQC2.AbstractButton.IconOnly
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: delegate.openRequested(delegate.path)
        }
    }

    Dialogs.FolderDialog {
        id: dialog
        title: delegate.text
        currentFolder: Paths.folderUrl(delegate.path)
        onAccepted: delegate.folders.setFolder(delegate.key, selectedFolder)
    }
}
