// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * The Launcher Logs page's toolbar: Copy, Upload and Open Folder, then the
 * rarer ones in its overflow menu -- wrap, reload, and deleting one file or
 * all of them, each asked first.
 */
QtObject {
    id: actions

    // SettingsLauncherLogsPage.qml and its Clipboard.
    required property var page
    required property var clipboard

    readonly property list<Kirigami.Action> list: [
        Kirigami.Action {
            text: i18nc("@action:button", "Copy")
            icon.name: "edit-copy"
            enabled: actions.page.hasFile
            onTriggered: {
                actions.clipboard.copyText(actions.page.logs.content)
                actions.page.notify(i18n("Log copied."))
            }
        },
        Kirigami.Action {
            text: actions.page.uploading ? i18nc("@action:button", "Uploading…") : i18nc("@action:button", "Upload")
            icon.name: "cloud-upload"
            visible: !!actions.page.uploader
            enabled: actions.page.hasFile && !!actions.page.uploader && !actions.page.uploader.busy
            onTriggered: {
                actions.page.uploading = true
                actions.page.uploader.uploadFile(actions.page.logs.path + "/" + actions.page.logs.currentFile)
            }
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Open Folder")
            icon.name: "document-open-folder"
            enabled: !!actions.page.logs
            onTriggered: actions.page.app.call("openPath", actions.page.logs.path)
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Wrap Lines")
            icon.name: "text-wrap"
            checkable: true
            checked: actions.page.wrap
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onToggled: actions.page.wrap = checked
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Reload")
            icon.name: "view-refresh"
            enabled: actions.page.hasFile
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: actions.page.logs.reload()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Delete This File…")
            icon.name: "edit-delete"
            enabled: actions.page.hasFile
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: deleteOne.open()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Delete All…")
            icon.name: "edit-delete"
            enabled: actions.page.files.length > 0
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: deleteAll.open()
        }
    ]

    readonly property Kirigami.PromptDialog deleteOne: Kirigami.PromptDialog {
        parent: actions.page
        title: i18nc("@title:dialog", "Delete Log File")
        subtitle: actions.page.hasFile ? i18n("Delete “%1”? It cannot be brought back from MeshMC.",
                                              actions.page.logs.currentFile) : ""
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Delete")
                icon.name: "edit-delete"
                onTriggered: {
                    actions.deleteOne.close()
                    if (!actions.page.logs.deleteCurrent())
                        actions.page.notify(i18n("The file could not be deleted."))
                }
            }
        ]
    }
    readonly property Kirigami.PromptDialog deleteAll: Kirigami.PromptDialog {
        parent: actions.page
        title: i18nc("@title:dialog", "Delete Logs")
        subtitle: i18np("Delete the log file? It cannot be brought back from MeshMC.",
                        "Delete all %1 log files? They cannot be brought back from MeshMC.",
                        actions.page.files.length)
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Delete All")
                icon.name: "edit-delete"
                onTriggered: {
                    actions.deleteAll.close()
                    if (actions.page.logs.deleteAll().length > 0)
                        actions.page.notify(i18n("Some files could not be deleted. They may be in use."))
                }
            }
        ]
    }
}
