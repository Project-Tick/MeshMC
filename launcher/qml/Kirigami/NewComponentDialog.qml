// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * "Add Empty Component": a name and an id. The component starts with no
 * libraries or arguments; they are filled in by editing its JSON
 * afterwards. The id is what the launcher knows it by, so it must be new to
 * this instance; left empty, one is made from the name.
 */
Kirigami.Dialog {
    id: dialog

    // ComponentTools (InstanceDetails.componentTools).
    property var tools: null

    readonly property string suggestedUid: dialog.tools ? dialog.tools.suggestUid(nameField.text) : ""
    readonly property string problem: dialog.tools
                                      ? dialog.tools.validateNewComponent(nameField.text, uidField.text, dialog.tools.componentUids())
                                      : ""
    // Shown once something has been typed, not on an untouched form.
    readonly property bool touched: nameField.text.length > 0 || uidField.text.length > 0

    title: i18nc("@title:dialog", "Add Empty Component")
    preferredWidth: Kirigami.Units.gridUnit * 28
    standardButtons: Kirigami.Dialog.Cancel
    customFooterActions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Add")
            icon.name: "list-add"
            enabled: dialog.problem.length === 0
            onTriggered: dialog.add()
        }
    ]

    function add() {
        if (dialog.problem.length === 0 && dialog.tools.addEmpty(nameField.text, uidField.text).length === 0)
            dialog.close()
    }
    function start() {
        nameField.text = ""
        uidField.text = ""
        dialog.open()
        nameField.forceActiveFocus()
    }

    ColumnLayout {
        spacing: 0

        FormCard.FormTextDelegate {
            text: i18n("An empty component has no files of its own. Add its libraries and arguments by editing its JSON afterwards.")
        }
        FormCard.FormTextFieldDelegate {
            id: nameField
            label: i18nc("@label:textbox", "Name")
            placeholderText: i18n("My component")
            onAccepted: dialog.add()
        }
        FormCard.FormTextFieldDelegate {
            id: uidField
            label: i18nc("@label:textbox", "ID")
            placeholderText: dialog.suggestedUid.length > 0 ? dialog.suggestedUid : "org.example.mycomponent"
            onAccepted: dialog.add()
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: dialog.touched && dialog.problem.length > 0
            type: Kirigami.MessageType.Error
            text: dialog.problem
        }
    }
}
