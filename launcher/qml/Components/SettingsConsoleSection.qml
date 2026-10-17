// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * The "Console" section of SettingsPage.qml: what the game's log window
 * does while you play, and how much of the log is kept.
 */
SettingsScroll {
    title: i18n("Console")
    description: i18n("What the game's log window does while you play, and how much of it is kept.")
    SettingsGroup {
        width: parent.width
        title: i18n("Game console")
        SettingSwitch {
            key: "ShowConsole"
            label: i18n("Show the console while playing")
        }
        SettingSwitch {
            key: "AutoCloseConsole"
            label: i18n("Close the console when the game quits")
        }
        SettingSwitch {
            key: "ShowConsoleOnError"
            label: i18n("Show the console when the game crashes")
        }
    }
    SettingsGroup {
        width: parent.width
        title: i18n("Font")
        ConsoleFontRow {
            label: i18n("Font")
            description: i18n("Used for the game's log. A monospaced font lines columns up best.")
        }
        SettingNumber {
            key: "ConsoleFontSize"
            label: i18n("Font size")
            from: 6
            to: 48
            suffix: i18n("pt")
        }
        ConsolePreview {}
    }
    SettingsGroup {
        width: parent.width
        title: i18n("Log history")
        SettingNumber {
            key: "ConsoleMaxLines"
            label: i18n("Lines to keep")
            from: 10000
            to: 1000000
            stepSize: 10000
        }
        SettingSwitch {
            key: "ConsoleOverflowStop"
            label: i18n("Stop logging when the limit is reached")
            description: i18n("Otherwise the oldest lines are dropped to make room.")
        }
    }
}
