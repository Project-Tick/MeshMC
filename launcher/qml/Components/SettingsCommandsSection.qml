// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * The "Custom commands" section of SettingsPage.qml: shell commands run
 * around every launch, launcher-wide. Each instance can override them on
 * its own Settings tab.
 */
SettingsScroll {
    title: i18n("Custom commands")
    description: i18n("Shell commands that run for every instance. Each instance can replace them in its own settings.")
    SettingsGroup {
        width: parent.width
        description: i18n("You can use $INST_NAME, $INST_ID, $INST_DIR, $INST_MC_DIR, $INST_JAVA and $INST_JAVA_ARGS in them.")
        SettingText {
            key: "PreLaunchCommand"
            label: i18n("Before launch")
            monospace: true
        }
        SettingText {
            key: "WrapperCommand"
            label: i18n("Wrapper")
            description: i18n("Runs the game through another program, such as prime-run or gamemoderun.")
            monospace: true
        }
        SettingText {
            key: "PostExitCommand"
            label: i18n("After exit")
            monospace: true
        }
    }
}
