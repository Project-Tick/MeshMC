// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

// The game window's size, or maximized (see OverridableCard.qml).
OverridableCard {
    id: card

    overrideKeys: ["LaunchMaximized", "MinecraftWinWidth", "MinecraftWinHeight"]

    SettingSwitch {
        id: maximized
        source: card.source
        key: "LaunchMaximized"
        enabled: card.editable
        text: i18nc("@option:check", "Start maximized")
    }
    FormCard.FormDelegateSeparator {}
    SettingNumber {
        source: card.source
        key: "MinecraftWinWidth"
        enabled: card.editable && !maximized.checked
        label: i18nc("@label:spinbox", "Window width")
        from: 1
        to: 65536
        suffix: i18nc("@item:spinbox unit", "px")
    }
    FormCard.FormDelegateSeparator {}
    SettingNumber {
        source: card.source
        key: "MinecraftWinHeight"
        enabled: card.editable && !maximized.checked
        label: i18nc("@label:spinbox", "Window height")
        from: 1
        to: 65536
        suffix: i18nc("@item:spinbox unit", "px")
    }
}
