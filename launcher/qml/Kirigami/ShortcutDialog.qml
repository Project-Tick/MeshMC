// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * "Create Shortcut…": a desktop or menu shortcut that starts one instance
 * straight away, optionally joining a server and playing as another
 * account. The writing is InstanceShortcutController.create(); "Somewhere
 * else…" asks for the file with a save dialog first.
 */
Kirigami.Dialog {
    id: dialog

    required property var app
    // The InstanceShortcutController of the instance.
    property var controller: null

    readonly property var locations: dialog.controller ? dialog.controller.locations : []
    readonly property var accounts: dialog.controller ? dialog.controller.accounts : []
    readonly property string location: placeField.currentValue || "other"
    readonly property string server: serverBox.checked ? serverField.text.trim() : ""
    readonly property bool complete: !serverBox.checked || dialog.server.length > 0

    function start(controller) {
        dialog.controller = controller
        nameField.text = ""
        serverBox.checked = false
        serverField.text = ""
        accountBox.checked = false
        accountField.currentIndex = Math.max(0, dialog.accounts.indexOf(controller.defaultAccount))
        placeField.currentIndex = 0
        failure.text = ""
        dialog.open()
    }

    function create(where) {
        var account = accountBox.checked ? accountField.currentText : ""
        if (!dialog.controller.create(nameField.text, where, dialog.server, account)) {
            failure.text = dialog.controller.error
            return
        }
        dialog.close()
        var controller = dialog.controller
        dialog.app.notify(i18n("Shortcut created."), i18nc("@action:button", "Show in Folder"),
                          () => controller.showInFolder())
    }

    title: i18nc("@title:dialog", "Create Shortcut")
    padding: Kirigami.Units.largeSpacing
    preferredWidth: Kirigami.Units.gridUnit * 26
    standardButtons: Kirigami.Dialog.Cancel
    customFooterActions: [
        Kirigami.Action {
            text: dialog.location === "other" ? i18nc("@action:button", "Create…") : i18nc("@action:button", "Create")
            icon.name: "link"
            enabled: dialog.complete
            onTriggered: {
                if (dialog.location !== "other") {
                    dialog.create(dialog.location)
                    return
                }
                shortcutFile.selectedFile = dialog.controller.suggestedFileUrl(
                            nameField.text.length > 0 ? nameField.text : nameField.placeholderText)
                shortcutFile.open()
            }
        }
    ]

    Kirigami.FormLayout {
        QQC2.TextField {
            id: nameField
            Kirigami.FormData.label: i18nc("@label:textbox", "Name:")
            // Follows the server until a name is typed.
            placeholderText: dialog.controller ? dialog.controller.suggestedName(dialog.server) : ""
        }
        QQC2.ComboBox {
            id: placeField
            Kirigami.FormData.label: i18nc("@label:listbox", "Place it in:")
            model: dialog.locations
            textRole: "label"
            valueRole: "value"
        }
        QQC2.CheckBox {
            id: serverBox
            text: i18nc("@option:check", "Join a server when it starts")
        }
        QQC2.TextField {
            id: serverField
            visible: serverBox.checked
            placeholderText: i18n("Server address, for example play.example.net")
        }
        QQC2.CheckBox {
            id: accountBox
            visible: dialog.accounts.length > 1
            text: i18nc("@option:check", "Play as a different account")
        }
        QQC2.ComboBox {
            id: accountField
            visible: accountBox.visible && accountBox.checked
            model: dialog.accounts
        }
        Kirigami.InlineMessage {
            id: failure
            Kirigami.FormData.isSection: true
            Layout.fillWidth: true
            visible: text.length > 0
            type: Kirigami.MessageType.Error
        }
    }

    Dialogs.FileDialog {
        id: shortcutFile
        title: i18nc("@title:dialog", "Create Shortcut")
        fileMode: Dialogs.FileDialog.SaveFile
        defaultSuffix: dialog.controller ? dialog.controller.fileSuffix.replace(/^\./, "") : ""
        onAccepted: dialog.create(selectedFile.toString())
    }
}
