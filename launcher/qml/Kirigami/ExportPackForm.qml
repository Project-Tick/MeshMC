// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * Export's format and, for a pack, what the pack says about itself: its
 * name and version, its author (CurseForge) or summary (Modrinth), whether
 * mods switched off in the instance come along as optional, and the memory
 * a CurseForge pack recommends. Every field writes straight to the
 * InstanceExportController.
 */
Kirigami.FormLayout {
    id: form

    // InstanceExportController.
    required property var controller
    readonly property string format: form.controller ? form.controller.format : "zip"
    readonly property bool pack: form.format !== "zip"

    readonly property int memoryStepMiB: 512
    readonly property int memoryMinMiB: 512
    readonly property int memoryMaxMiB: 65536

    // Checked by the controller's format alone, which can also change
    // from outside (an export opened on a format).
    component FormatButton: QQC2.RadioButton {
        required property string formatId
        autoExclusive: false
        checked: form.format === formatId
        onToggled: {
            form.controller.format = formatId
            checked = Qt.binding(() => form.format === formatId)
        }
    }

    FormatButton {
        Kirigami.FormData.label: i18nc("@label", "Format:")
        formatId: "zip"
        text: i18nc("@option:radio export format", "Zip archive")
    }
    // Only a Minecraft instance has the mods a pack lists.
    FormatButton {
        visible: !!form.controller && form.controller.packFormats
        formatId: "modrinth"
        text: i18nc("@option:radio export format", "Modrinth pack (.mrpack)")
    }
    FormatButton {
        visible: !!form.controller && form.controller.packFormats
        formatId: "curseforge"
        text: i18nc("@option:radio export format", "CurseForge pack")
    }
    QQC2.Label {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 20
        wrapMode: Text.Wrap
        color: Kirigami.Theme.disabledTextColor
        text: form.format === "modrinth"
              ? i18n("Mods from Modrinth are linked for download, and everything else is packed in.")
              : form.format === "curseforge"
                ? i18n("Mods from CurseForge are linked for download, and everything else is packed in.")
                : i18n("The whole instance in one file, ready to import into MeshMC or another launcher.")
    }

    Kirigami.Separator {
        visible: form.pack
        Kirigami.FormData.isSection: true
    }
    QQC2.TextField {
        visible: form.pack
        Kirigami.FormData.label: i18nc("@label:textbox", "Name:")
        text: form.controller ? form.controller.packName : ""
        placeholderText: form.controller ? form.controller.instanceName : ""
        onTextEdited: form.controller.packName = text
    }
    QQC2.TextField {
        visible: form.pack
        Kirigami.FormData.label: i18nc("@label:textbox", "Version:")
        text: form.controller ? form.controller.packVersion : ""
        placeholderText: "1.0.0"
        onTextEdited: form.controller.packVersion = text
    }
    QQC2.TextField {
        visible: form.format === "curseforge"
        Kirigami.FormData.label: i18nc("@label:textbox", "Author:")
        text: form.controller ? form.controller.packAuthor : ""
        onTextEdited: form.controller.packAuthor = text
    }
    QQC2.TextField {
        visible: form.format === "modrinth"
        Kirigami.FormData.label: i18nc("@label:textbox", "Summary:")
        text: form.controller ? form.controller.packSummary : ""
        placeholderText: i18nc("@info:placeholder", "One line about the pack")
        onTextEdited: form.controller.packSummary = text
    }
    QQC2.CheckBox {
        visible: form.pack
        text: i18nc("@option:check", "Mark disabled mods as optional")
        checked: !!form.controller && form.controller.includeOptional
        onToggled: form.controller.includeOptional = checked
    }
    RowLayout {
        visible: form.format === "curseforge"
        spacing: Kirigami.Units.smallSpacing
        QQC2.CheckBox {
            id: memoryBox
            text: i18nc("@option:check", "Recommend memory:")
            checked: !!form.controller && form.controller.recommendMemory
            onToggled: form.controller.recommendMemory = checked
        }
        QQC2.SpinBox {
            enabled: memoryBox.checked
            editable: true
            from: form.memoryMinMiB
            to: form.memoryMaxMiB
            stepSize: form.memoryStepMiB
            value: form.controller ? form.controller.recommendedMemory : form.memoryMinMiB
            textFromValue: (value, locale) => i18nc("@item:valuesuffix", "%1 MiB", Number(value).toLocaleString(locale, "f", 0))
            valueFromText: (text, locale) => Number.fromLocaleString(locale, text.replace(/[^0-9.,]/g, ""))
            onValueModified: form.controller.recommendedMemory = value
            Accessible.name: i18nc("@label:spinbox", "Recommended memory")
        }
    }
    QQC2.Label {
        visible: form.format === "modrinth" && !!form.controller && !form.controller.ready
        Kirigami.FormData.label: ""
        text: i18n("A Modrinth pack needs a version.")
        color: Kirigami.Theme.negativeTextColor
    }
}
