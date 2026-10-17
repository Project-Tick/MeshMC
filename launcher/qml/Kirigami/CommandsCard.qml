// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

// Shell commands run around a launch (see OverridableCard.qml).
OverridableCard {
    id: card

    overrideKeys: ["PreLaunchCommand", "WrapperCommand", "PostExitCommand"]

    SettingText {
        source: card.source
        key: "PreLaunchCommand"
        enabled: card.editable
        label: i18nc("@label:textbox", "Before launch")
    }
    FormCard.FormDelegateSeparator {}
    SettingText {
        source: card.source
        key: "WrapperCommand"
        enabled: card.editable
        label: i18nc("@label:textbox", "Wrapper")
        description: i18n("Runs the game through another program, such as prime-run or gamemoderun.")
    }
    FormCard.FormDelegateSeparator {}
    SettingText {
        source: card.source
        key: "PostExitCommand"
        enabled: card.editable
        label: i18nc("@label:textbox", "After exit")
    }
}
