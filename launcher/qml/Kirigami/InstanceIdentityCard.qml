// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * What a new instance is called, the group it goes in and its icon: the
 * first card of New Instance and Import alike.
 *
 * The page suggests a name (suggest()) until one is typed; clearing the
 * field hands it back to the suggestions.
 */
FormCard.FormCard {
    id: card

    required property var app
    property string placeholderName
    property string iconKey: "default"

    readonly property string name: nameField.text.trim()
    readonly property string group: groupField.editText.trim()
    property bool nameEdited: false

    // The page's suggestion, taken unless a name was typed.
    function suggest(name) {
        if (!card.nameEdited)
            nameField.text = name
    }
    function focusName() {
        nameField.forceActiveFocus()
    }

    FormCard.FormTextFieldDelegate {
        id: nameField
        label: i18nc("@label:textbox", "Name")
        placeholderText: card.placeholderName
        onTextEdited: card.nameEdited = text.length > 0
    }
    FormCard.FormDelegateSeparator {}
    FormCard.FormComboBoxDelegate {
        id: groupField
        text: i18nc("@label:listbox", "Group")
        editable: true
        model: card.app.instanceGroups().filter(g => g.length > 0)
        Component.onCompleted: groupField.editText = ""
    }
    FormCard.FormDelegateSeparator {}
    FormCard.FormButtonDelegate {
        text: i18nc("@action:button", "Icon")
        description: i18n("What the library shows for it")
        leading: Image {
            source: "image://instanceicon/" + card.iconKey
            sourceSize.width: Kirigami.Units.iconSizes.medium
            sourceSize.height: Kirigami.Units.iconSizes.medium
            fillMode: Image.PreserveAspectFit
            smooth: false
        }
        onClicked: {
            iconPicker.current = card.iconKey
            iconPicker.open()
        }
    }

    IconPickerDialog {
        id: iconPicker
        shell: card.app.shell
        onPicked: (key) => card.iconKey = key
    }
}
