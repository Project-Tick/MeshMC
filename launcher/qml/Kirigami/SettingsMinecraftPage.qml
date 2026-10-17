// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * Minecraft: the game window, play time, and native library overrides,
 * launcher-wide. The game window cards are shared with an instance's own
 * settings (GameWindowCard.qml).
 */
SettingsPage {
    id: page

    FormCard.FormHeader {
        title: i18nc("@title:group", "Game Window")
    }
    GameWindowCard {
        source: page.store
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Play Time")
    }
    FormCard.FormCard {
        SettingSwitch {
            source: page.store
            key: "RecordGameTime"
            text: i18nc("@option:check", "Record play time")
        }
        FormCard.FormDelegateSeparator {}
        SettingSwitch {
            source: page.store
            key: "ShowGameTime"
            text: i18nc("@option:check", "Show play time for each instance")
        }
        FormCard.FormDelegateSeparator {}
        SettingSwitch {
            source: page.store
            key: "ShowGlobalGameTime"
            text: i18nc("@option:check", "Show total play time")
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Native Libraries")
    }
    NativeLibrariesCard {
        source: page.store
    }
}
