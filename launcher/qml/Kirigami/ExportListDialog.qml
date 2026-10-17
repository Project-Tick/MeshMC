// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * "Export List": the instance's mods (or packs) as text to paste or keep --
 * HTML, Markdown, plain text, JSON, CSV, or a line of your own with {name},
 * {version} and friends filled in (the widget ExportListDialog). The text
 * comes from ContentListExport through InstanceContentController's
 * exportText(), so both interfaces write the same thing, and the preview is
 * exactly what is copied or saved.
 */
Kirigami.Dialog {
    id: dialog

    // InstanceContentController, and which of its kinds to export.
    property var controller: null
    property string kind: "mods"
    // The instance's name, for the suggested file name.
    property string listName: ""
    signal done(string message)

    property string format: "markdown"
    property var fields: ["version", "url"]
    property string lineTemplate: ""
    // Bumped on open, so the preview reads files added since.
    property int revision: 0
    readonly property bool custom: dialog.format === "custom"
    readonly property string output: {
        dialog.revision
        return dialog.controller ? dialog.controller.exportText(dialog.kind, [], dialog.format,
                                                                dialog.fields, dialog.lineTemplate) : ""
    }

    readonly property var formats: [
        { value: "html", text: i18nc("@item:inlistbox", "HTML") },
        { value: "markdown", text: i18nc("@item:inlistbox", "Markdown") },
        { value: "plaintext", text: i18nc("@item:inlistbox", "Plain text") },
        { value: "json", text: i18nc("@item:inlistbox", "JSON") },
        { value: "csv", text: i18nc("@item:inlistbox", "CSV") },
        { value: "custom", text: i18nc("@item:inlistbox", "Custom") }
    ]
    readonly property var fieldOptions: [
        { value: "version", text: i18nc("@option:check", "Version") },
        { value: "authors", text: i18nc("@option:check", "Authors") },
        { value: "url", text: i18nc("@option:check", "Link") },
        { value: "filename", text: i18nc("@option:check", "File name") }
    ]
    readonly property var placeholders: [
        { value: "{name}", text: i18nc("@action:button", "Name") },
        { value: "{version}", text: i18nc("@action:button", "Version") },
        { value: "{authors}", text: i18nc("@action:button", "Authors") },
        { value: "{url}", text: i18nc("@action:button", "Link") },
        { value: "{filename}", text: i18nc("@action:button", "File name") },
        { value: "{mod_id}", text: i18nc("@action:button", "Mod ID") }
    ]

    function toggleField(value, on) {
        var list = dialog.fields.filter(f => f !== value)
        if (on)
            list.push(value)
        dialog.fields = list
    }

    title: i18nc("@title:dialog", "Export List")
    preferredWidth: Kirigami.Units.gridUnit * 34
    maximumHeight: applicationWindow().height - Kirigami.Units.gridUnit * 4
    standardButtons: Kirigami.Dialog.Cancel
    customFooterActions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Copy")
            icon.name: "edit-copy"
            enabled: dialog.output.length > 0
            onTriggered: {
                clipboard.copyText(dialog.output)
                dialog.close()
                dialog.done(i18n("The list is on the clipboard."))
            }
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Save…")
            icon.name: "document-save"
            enabled: dialog.output.length > 0
            onTriggered: {
                saveDialog.selectedFile = dialog.controller.suggestedExportUrl(dialog.listName, dialog.format)
                saveDialog.open()
            }
        }
    ]

    onOpened: dialog.revision++

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.FormLayout {
            Layout.fillWidth: true

            QQC2.ComboBox {
                id: formatBox
                Kirigami.FormData.label: i18nc("@label:listbox", "Format:")
                model: dialog.formats
                textRole: "text"
                valueRole: "value"
                Component.onCompleted: currentIndex = indexOfValue(dialog.format)
                onActivated: {
                    // A custom line starts from the format it replaces.
                    if (currentValue === "custom" && dialog.lineTemplate.length === 0 && dialog.controller)
                        dialog.lineTemplate = dialog.controller.exampleLine(dialog.format)
                    dialog.format = currentValue
                }
            }
            Flow {
                Kirigami.FormData.label: i18nc("@label", "Include:")
                Layout.fillWidth: true
                visible: !dialog.custom
                spacing: Kirigami.Units.largeSpacing
                Repeater {
                    model: dialog.fieldOptions
                    QQC2.CheckBox {
                        required property var modelData
                        text: modelData.text
                        checked: dialog.fields.indexOf(modelData.value) >= 0
                        onToggled: dialog.toggleField(modelData.value, checked)
                    }
                }
            }
            QQC2.TextField {
                id: templateField
                Kirigami.FormData.label: i18nc("@label:textbox", "Each line:")
                Layout.fillWidth: true
                visible: dialog.custom
                text: dialog.lineTemplate
                placeholderText: i18nc("@info:placeholder", "For example {name} {version}")
                font: Kirigami.Theme.fixedWidthFont
                onTextEdited: dialog.lineTemplate = text
            }
            Flow {
                Kirigami.FormData.label: i18nc("@label", "Insert:")
                Layout.fillWidth: true
                visible: dialog.custom
                spacing: Kirigami.Units.smallSpacing
                Repeater {
                    model: dialog.placeholders
                    QQC2.Button {
                        required property var modelData
                        text: modelData.text
                        icon.name: "list-add"
                        flat: true
                        onClicked: {
                            templateField.insert(templateField.cursorPosition, modelData.value)
                            dialog.lineTemplate = templateField.text
                            templateField.forceActiveFocus()
                        }
                        QQC2.ToolTip.text: i18nc("@info:tooltip %1 is a placeholder such as {name}", "Insert %1", modelData.value)
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }
                }
            }
        }

        // Exactly what is copied or saved.
        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 10
            QQC2.TextArea {
                readOnly: true
                text: dialog.output
                wrapMode: TextEdit.NoWrap
                font: Kirigami.Theme.fixedWidthFont
                Accessible.name: i18nc("@label", "Preview")
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: i18n("Built from what each file says about itself, so some may have no link or authors.")
            wrapMode: Text.Wrap
            opacity: 0.7
        }

        // Inside the one content item: the dialog sizes itself from its
        // only child.
        Clipboard {
            id: clipboard
        }
    }

    Dialogs.FileDialog {
        id: saveDialog
        title: i18nc("@title:dialog", "Save List")
        fileMode: Dialogs.FileDialog.SaveFile
        defaultSuffix: dialog.controller ? dialog.controller.exportExtension(dialog.format).slice(1) : "txt"
        nameFilters: dialog.controller
                     ? [i18nc("@item:inlistbox %1 a format, %2 its file extension", "%1 files (*%2)", formatBox.currentText,
                              dialog.controller.exportExtension(dialog.format)),
                        i18nc("@item:inlistbox", "All files (*)")]
                     : [i18nc("@item:inlistbox", "All files (*)")]
        onAccepted: {
            if (dialog.controller.saveText(selectedFile, dialog.output)) {
                dialog.close()
                dialog.done(i18n("Saved the list."))
            } else {
                dialog.done(dialog.controller.lastError)
            }
        }
    }
}
