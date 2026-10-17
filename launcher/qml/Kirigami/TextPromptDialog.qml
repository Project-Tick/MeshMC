// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * Asks for one line of text: a skin's link, a player's name, a new name.
 * submitted() hands it over; the caller closes the dialog, or sets `error`
 * to say what is wrong and leaves it open.
 */
Kirigami.PromptDialog {
    id: dialog

    property string confirmText: i18nc("@action:button", "OK")
    property alias placeholderText: field.placeholderText
    property alias value: field.text
    property string error: ""

    signal submitted(string text)

    function ask(initial) {
        field.text = initial || ""
        dialog.error = ""
        dialog.open()
        field.forceActiveFocus()
        field.selectAll()
    }

    preferredWidth: Kirigami.Units.gridUnit * 22
    standardButtons: Kirigami.Dialog.Cancel
    customFooterActions: [
        Kirigami.Action {
            text: dialog.confirmText
            icon.name: "dialog-ok"
            enabled: field.text.trim().length > 0
            onTriggered: dialog.submitted(field.text.trim())
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        QQC2.TextField {
            id: field
            Layout.fillWidth: true
            onTextEdited: dialog.error = ""
            onAccepted: if (text.trim().length > 0) dialog.submitted(text.trim())
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: dialog.error.length > 0
            type: Kirigami.MessageType.Error
            text: dialog.error
        }
    }
}
