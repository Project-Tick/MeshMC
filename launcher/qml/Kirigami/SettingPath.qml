// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import "paths.js" as Paths

/*
 * A path setting: typed, or picked with Browse from the system's own file
 * dialog -- a folder with `folder`, otherwise one file. With `toolId`
 * ("jprofiler", "jvisualvm", "mcedit") a Check button asks the settings
 * adapter whether the tool is really there, and says so under the field.
 */
SettingText {
    id: delegate

    property bool folder: false
    property var nameFilters: [i18n("All files (*)")]
    property string toolId: ""

    function save(path) {
        delegate.text = path
        delegate.cursorPosition = 0
        delegate.source.setValue(delegate.key, path)
        delegate.statusMessage = ""
    }

    function check() {
        var adapter = delegate.source.adapter
        var error = adapter && typeof adapter.checkExternalTool === "function"
                ? adapter.checkExternalTool(delegate.toolId, delegate.text) : i18n("Cannot check this here.")
        delegate.status = error.length === 0 ? Kirigami.MessageType.Positive : Kirigami.MessageType.Error
        delegate.statusMessage = error.length === 0 ? i18n("Found and ready to use.") : error
    }

    onTextEdited: delegate.statusMessage = ""

    trailing: [
        QQC2.Button {
            icon.name: "document-open-folder"
            text: i18nc("@action:button", "Browse…")
            display: QQC2.AbstractButton.IconOnly
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: delegate.folder ? folderDialog.open() : fileDialog.open()
        },
        QQC2.Button {
            visible: delegate.toolId.length > 0
            text: i18nc("@action:button", "Check")
            enabled: delegate.text.length > 0
            onClicked: delegate.check()
        }
    ]

    Dialogs.FolderDialog {
        id: folderDialog
        title: delegate.label
        currentFolder: Paths.folderUrl(delegate.text)
        onAccepted: delegate.save(Paths.localPath(selectedFolder))
    }
    Dialogs.FileDialog {
        id: fileDialog
        title: delegate.label
        nameFilters: delegate.nameFilters
        onAccepted: delegate.save(Paths.localPath(selectedFile))
    }
}
