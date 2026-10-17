// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

// When the game's console shows and closes (see OverridableCard.qml).
OverridableCard {
    id: card

    overrideKeys: ["ShowConsole", "AutoCloseConsole", "ShowConsoleOnError"]

    SettingSwitch {
        source: card.source
        key: "ShowConsole"
        enabled: card.editable
        text: i18nc("@option:check", "Show the console while playing")
    }
    FormCard.FormDelegateSeparator {}
    SettingSwitch {
        source: card.source
        key: "AutoCloseConsole"
        enabled: card.editable
        text: i18nc("@option:check", "Close the console when the game quits")
    }
    FormCard.FormDelegateSeparator {}
    SettingSwitch {
        source: card.source
        key: "ShowConsoleOnError"
        enabled: card.editable
        text: i18nc("@option:check", "Show the console when the game crashes")
    }
}
