// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * Add or edit one server of servers.dat: the three things the list keeps --
 * its name, its address, and what to do about the server's resource pack.
 */
Kirigami.Dialog {
    id: dialog

    // ServersListModel (InstanceDetails.servers).
    property var model: null
    // The server being edited; -1 adds a new one.
    property int row: -1

    signal saved(int row)

    // servers.dat's per-server resource pack choice, in its own order.
    readonly property var packChoices: [
        { value: 0, text: i18nc("@item:inlistbox server resource packs", "Ask") },
        { value: 1, text: i18nc("@item:inlistbox server resource packs", "Always") },
        { value: 2, text: i18nc("@item:inlistbox server resource packs", "Never") }
    ]
    readonly property bool valid: addressField.text.trim().length > 0

    title: dialog.row < 0 ? i18nc("@title:dialog", "Add Server") : i18nc("@title:dialog", "Edit Server")
    preferredWidth: Kirigami.Units.gridUnit * 28
    standardButtons: Kirigami.Dialog.Cancel
    customFooterActions: [
        Kirigami.Action {
            text: dialog.row < 0 ? i18nc("@action:button", "Add Server") : i18nc("@action:button", "Save")
            icon.name: dialog.row < 0 ? "list-add" : "document-save"
            enabled: dialog.valid
            onTriggered: dialog.save()
        }
    ]

    // Adds a server when @p row is -1.
    function edit(row, name, address, acceptTextures) {
        dialog.row = row
        nameField.text = row < 0 ? "" : name
        addressField.text = row < 0 ? "" : address
        packBox.currentIndex = row < 0 ? 0 : Math.max(0, acceptTextures)
        dialog.open()
        nameField.forceActiveFocus()
    }
    function save() {
        if (!dialog.model || !dialog.valid)
            return
        var target = dialog.row
        if (target < 0) {
            target = dialog.model.addServer()
            if (target < 0)
                return
        }
        dialog.model.setName(target, nameField.text.trim())
        dialog.model.setAddress(target, addressField.text.trim())
        dialog.model.setAcceptTextures(target, dialog.packChoices[packBox.currentIndex].value)
        dialog.saved(target)
        dialog.close()
    }

    ColumnLayout {
        spacing: 0

        FormCard.FormTextFieldDelegate {
            id: nameField
            label: i18nc("@label:textbox", "Name")
            placeholderText: i18n("Minecraft Server")
            onAccepted: addressField.forceActiveFocus()
        }
        FormCard.FormTextFieldDelegate {
            id: addressField
            label: i18nc("@label:textbox", "Address")
            placeholderText: i18n("play.example.com")
            onAccepted: dialog.save()
        }
        FormCard.FormComboBoxDelegate {
            id: packBox
            text: i18nc("@label:listbox", "Server resource packs")
            model: dialog.packChoices
            textRole: "text"
            valueRole: "value"
        }
    }
}
