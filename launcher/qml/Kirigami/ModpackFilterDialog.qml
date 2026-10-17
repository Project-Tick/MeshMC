// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * Discover's filters: which mod loader the packs are built on. Each choice
 * searches again at once, so there is no Apply. Providers that say nothing
 * about loaders (ATLauncher, Technic) have no Filters at all.
 */
Kirigami.Dialog {
    id: dialog

    // The provider's modpack model: `loader`, and search().
    property var model: null
    signal changed()

    readonly property var loaders: [
        { value: "", text: i18nc("@option:radio any mod loader", "Any") },
        { value: "fabric", text: "Fabric" },
        { value: "forge", text: "Forge" },
        { value: "neoforge", text: "NeoForge" },
        { value: "quilt", text: "Quilt" }
    ]
    readonly property string loader: dialog.model ? dialog.model.loader : ""

    title: i18nc("@title:dialog", "Filters")
    preferredWidth: Kirigami.Units.gridUnit * 18
    standardButtons: Kirigami.Dialog.Close
    customFooterActions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Reset")
            icon.name: "edit-undo"
            enabled: dialog.loader.length > 0
            onTriggered: dialog.pick("")
        }
    ]

    function pick(value) {
        if (!dialog.model || dialog.model.loader === value)
            return
        dialog.model.loader = value
        dialog.changed()
    }

    Kirigami.FormLayout {
        QQC2.ButtonGroup {
            id: loaderGroup
        }
        Repeater {
            model: dialog.loaders
            QQC2.RadioButton {
                id: radio
                required property var modelData
                required property int index
                Kirigami.FormData.label: radio.index === 0 ? i18nc("@label", "Mod loader:") : ""
                text: radio.modelData.text
                checked: dialog.loader === radio.modelData.value
                QQC2.ButtonGroup.group: loaderGroup
                onToggled: {
                    if (checked)
                        dialog.pick(radio.modelData.value)
                    checked = Qt.binding(() => dialog.loader === radio.modelData.value)
                }
            }
        }
    }
}
