// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

// The system's own GLFW and OpenAL instead of the bundled ones (see
// OverridableCard.qml).
OverridableCard {
    id: card

    overrideKeys: ["UseNativeGLFW", "UseNativeOpenAL"]

    SettingSwitch {
        source: card.source
        key: "UseNativeGLFW"
        enabled: card.editable
        text: i18nc("@option:check", "Use the system's GLFW")
        description: i18n("Only for systems where the bundled libraries do not work, such as some Linux setups.")
    }
    FormCard.FormDelegateSeparator {}
    SettingSwitch {
        source: card.source
        key: "UseNativeOpenAL"
        enabled: card.editable
        text: i18nc("@option:check", "Use the system's OpenAL")
    }
}
