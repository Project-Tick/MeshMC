// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * "Export…": one instance out as a file -- the whole folder as a zip, or a
 * Modrinth or CurseForge pack, which links the mods those sites have and
 * packs in the rest. The work is InstanceExportController's, the classic
 * window's too, so both write the same archive and share the instance's
 * .packignore: what is unchecked here stays unchecked next time.
 *
 * The dialog stays up while the file is written, with how far it is and a
 * way to stop, and closes once it is there; a failure keeps it open with
 * the reason, so nothing has to be chosen again.
 */
Kirigami.Dialog {
    id: dialog

    required property var app
    // InstanceExportController (QmlShell's exportController()).
    property var controller: null
    // The running export's TaskWatcher.
    property var watcher: null
    property bool stopping: false
    property string failure

    readonly property bool exporting: !!dialog.watcher && dialog.watcher.running
    readonly property string format: dialog.controller ? dialog.controller.format : "zip"
    readonly property bool pack: dialog.format !== "zip"

    // @p format ("zip", "modrinth", "curseforge") is optional.
    function start(controller, format) {
        dialog.controller = controller
        if (format)
            controller.format = format
        dialog.watcher = null
        dialog.stopping = false
        dialog.failure = ""
        dialog.open()
    }
    function exportTo(fileUrl) {
        dialog.failure = ""
        dialog.stopping = false
        var watcher = dialog.controller.exportTo(String(fileUrl))
        if (!watcher)
            dialog.failure = dialog.controller.error
        dialog.watcher = watcher
    }

    title: dialog.controller ? i18nc("@title:dialog", "Export “%1”", dialog.controller.instanceName)
                             : i18nc("@title:dialog", "Export Instance")
    padding: Kirigami.Units.largeSpacing
    preferredWidth: Kirigami.Units.gridUnit * 32
    // Typing is not something a stray click outside should throw away, and
    // an export in progress stops only through its button.
    closePolicy: dialog.exporting ? QQC2.Popup.NoAutoClose : QQC2.Popup.CloseOnEscape
    showCloseButton: !dialog.exporting
    standardButtons: Kirigami.Dialog.NoButton
    // What was unchecked and typed is kept for next time, exported or not.
    onClosed: if (dialog.controller) dialog.controller.saveChoices()

    customFooterActions: [
        Kirigami.Action {
            visible: !dialog.exporting
            text: i18nc("@action:button", "Export…")
            icon.name: "document-export"
            enabled: !!dialog.controller && dialog.controller.ready
            onTriggered: {
                saveDialog.selectedFile = dialog.controller.suggestedFileUrl()
                saveDialog.open()
            }
        },
        Kirigami.Action {
            visible: dialog.exporting
            text: i18nc("@action:button", "Stop")
            icon.name: "process-stop"
            enabled: !dialog.stopping
            onTriggered: {
                dialog.stopping = true
                dialog.controller.cancel()
            }
        },
        Kirigami.Action {
            visible: !dialog.exporting
            text: i18nc("@action:button", "Cancel")
            icon.name: "dialog-cancel"
            onTriggered: dialog.close()
        }
    ]

    Connections {
        target: dialog.watcher
        function onFinished(ok) {
            var watcher = dialog.watcher
            dialog.watcher = null
            if (ok) {
                var path = dialog.controller.lastOutput
                var controller = dialog.controller
                dialog.close()
                dialog.app.notify(i18n("Saved “%1”.", path.substring(path.lastIndexOf("/") + 1)),
                                  i18nc("@action:button", "Show in Folder"), () => controller.showInFolder())
            } else if (!dialog.stopping) {
                dialog.failure = watcher.error
            }
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: !!dialog.controller && dialog.controller.running
            type: Kirigami.MessageType.Warning
            text: i18n("The game is running. Files it is writing now may be left out or cut short, so quit it first for a clean export.")
        }

        ExportPackForm {
            Layout.fillWidth: true
            controller: dialog.controller
            enabled: !dialog.exporting
        }

        Kirigami.ListSectionHeader {
            Layout.fillWidth: true
            text: i18nc("@title:group", "Files")
        }
        ExportFileTree {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 12
            controller: dialog.controller
            enabled: !dialog.exporting
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: i18n("Unchecked files and folders are left out.")
            wrapMode: Text.Wrap
            color: Kirigami.Theme.disabledTextColor
        }

        // How far the archive has got.
        ColumnLayout {
            Layout.fillWidth: true
            visible: dialog.exporting
            spacing: Kirigami.Units.smallSpacing
            QQC2.Label {
                Layout.fillWidth: true
                text: dialog.watcher && dialog.watcher.status.length > 0 ? dialog.watcher.status
                                                                       : i18nc("@info:status", "Exporting…")
                elide: Text.ElideMiddle
            }
            LaunchProgressBar {
                Layout.fillWidth: true
                progress: dialog.watcher ? dialog.watcher.progress : -1
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: dialog.failure.length > 0
            type: Kirigami.MessageType.Error
            text: dialog.failure
        }
    }

    Dialogs.FileDialog {
        id: saveDialog
        title: i18nc("@title:dialog", "Export To")
        fileMode: Dialogs.FileDialog.SaveFile
        defaultSuffix: dialog.format === "modrinth" ? "mrpack" : "zip"
        nameFilters: dialog.format === "modrinth" ? [i18n("Modrinth packs (*.mrpack)")]
                                                  : [i18n("Zip archives (*.zip)")]
        onAccepted: dialog.exportTo(selectedFile)
    }
}
