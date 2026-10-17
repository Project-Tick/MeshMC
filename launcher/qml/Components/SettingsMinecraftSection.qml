// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * The "Minecraft" section of SettingsPage.qml: the game window, play time
 * tracking, and native library overrides, launcher-wide.
 */
SettingsScroll {
    title: i18n("Minecraft")
    description: i18n("The game window, play time tracking, and native library overrides.")
    SettingsGroup {
        width: parent.width
        title: i18n("Game window")
        SettingSwitch {
            id: maximized
            key: "LaunchMaximized"
            label: i18n("Start maximized")
        }
        SettingNumber {
            key: "MinecraftWinWidth"
            enabled: !maximized.checked
            label: i18n("Window width")
            from: 1
            to: 65536
            suffix: i18n("px")
        }
        SettingNumber {
            key: "MinecraftWinHeight"
            enabled: !maximized.checked
            label: i18n("Window height")
            from: 1
            to: 65536
            suffix: i18n("px")
        }
    }
    SettingsGroup {
        width: parent.width
        title: i18n("Play time")
        SettingSwitch {
            key: "RecordGameTime"
            label: i18n("Record play time")
        }
        SettingSwitch {
            key: "ShowGameTime"
            label: i18n("Show play time per instance")
        }
        SettingSwitch {
            key: "ShowGlobalGameTime"
            label: i18n("Show total play time")
        }
    }
    SettingsGroup {
        width: parent.width
        title: i18n("Native libraries")
        description: i18n("Only for systems where the bundled libraries don't work, such as some Linux setups.")
        SettingSwitch {
            key: "UseNativeGLFW"
            label: i18n("Use the system's GLFW")
        }
        SettingSwitch {
            key: "UseNativeOpenAL"
            label: i18n("Use the system's OpenAL")
        }
    }
}
