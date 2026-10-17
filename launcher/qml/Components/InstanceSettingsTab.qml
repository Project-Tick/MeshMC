// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls
import MeshMC.Theme

/*
 * One instance's own settings: each group follows the launcher-wide value
 * until switched to "custom for this instance". Locked while the instance
 * runs -- the game already read these, a change now would silently do
 * nothing until the next launch. A TabScroll: the tab strip names the tab,
 * so there is no heading, and the groups run the page's full width.
 */
TabScroll {
    id: root

    // SettingsAdapter over the instance's settings.
    property var adapter: null
    property int systemMemoryMiB: 8192
    property bool running: false
    // PluginSurfaceModel for this instance's settings anchor, or null.
    property var pluginSurfaces: null

    description: i18n("Each group follows the launcher's settings until you switch it to custom for this instance.")

    SettingsSource {
        id: store
        adapter: root.adapter
    }

    NoticeBar {
        visible: root.running
        width: parent.width
        text: i18n("The game is running. Settings can be changed once it has closed.")
    }

    Column {
        width: parent.width
        spacing: Theme.space.xl
        enabled: !root.running

        // Memory, Java and JVM arguments: their own file, so Java
        // management can grow there without touching the rest of this tab.
        InstanceJavaSettings {
            width: parent.width
            source: store
            systemMemoryMiB: root.systemMemoryMiB
        }

        OverrideGroup {
            id: windowGroup
            width: parent.width
            title: i18n("Game window")
            source: store
            gateKey: "OverrideWindow"
            keys: ["LaunchMaximized", "MinecraftWinWidth", "MinecraftWinHeight"]
            SettingSwitch {
                id: maximized
                source: store
                key: "LaunchMaximized"
                enabled: windowGroup.overriding
                label: i18n("Start maximized")
            }
            SettingNumber {
                source: store
                key: "MinecraftWinWidth"
                enabled: windowGroup.overriding && !maximized.checked
                label: i18n("Window width")
                from: 1
                to: 65536
                suffix: i18n("px")
            }
            SettingNumber {
                source: store
                key: "MinecraftWinHeight"
                enabled: windowGroup.overriding && !maximized.checked
                label: i18n("Window height")
                from: 1
                to: 65536
                suffix: i18n("px")
            }
        }

        OverrideGroup {
            id: consoleGroup
            width: parent.width
            title: i18n("Console")
            source: store
            gateKey: "OverrideConsole"
            keys: ["ShowConsole", "AutoCloseConsole", "ShowConsoleOnError"]
            SettingSwitch {
                source: store
                key: "ShowConsole"
                enabled: consoleGroup.overriding
                label: i18n("Show the console while playing")
            }
            SettingSwitch {
                source: store
                key: "AutoCloseConsole"
                enabled: consoleGroup.overriding
                label: i18n("Close the console when the game quits")
            }
            SettingSwitch {
                source: store
                key: "ShowConsoleOnError"
                enabled: consoleGroup.overriding
                label: i18n("Show the console when the game crashes")
            }
        }

        OverrideGroup {
            id: commands
            width: parent.width
            title: i18n("Custom commands")
            source: store
            gateKey: "OverrideCommands"
            keys: ["PreLaunchCommand", "WrapperCommand", "PostExitCommand"]
            SettingText {
                source: store
                key: "PreLaunchCommand"
                enabled: commands.overriding
                label: i18n("Before launch")
                monospace: true
            }
            SettingText {
                source: store
                key: "WrapperCommand"
                enabled: commands.overriding
                label: i18n("Wrapper")
                monospace: true
            }
            SettingText {
                source: store
                key: "PostExitCommand"
                enabled: commands.overriding
                label: i18n("After exit")
                monospace: true
            }
        }

        OverrideGroup {
            id: gameTime
            width: parent.width
            title: i18n("Play time")
            source: store
            gateKey: "OverrideGameTime"
            keys: ["RecordGameTime", "ShowGameTime"]
            SettingSwitch {
                source: store
                key: "RecordGameTime"
                enabled: gameTime.overriding
                label: i18n("Record play time")
            }
            SettingSwitch {
                source: store
                key: "ShowGameTime"
                enabled: gameTime.overriding
                label: i18n("Show play time")
            }
        }

        OverrideGroup {
            id: workarounds
            width: parent.width
            title: i18n("Native libraries")
            source: store
            gateKey: "OverrideNativeWorkarounds"
            gateLabel: i18n("Use custom native library settings for this instance")
            keys: ["UseNativeGLFW", "UseNativeOpenAL"]
            SettingSwitch {
                source: store
                key: "UseNativeGLFW"
                enabled: workarounds.overriding
                label: i18n("Use the system's GLFW")
                description: i18n("Only for systems where the bundled libraries don't work, such as some Linux setups.")
            }
            SettingSwitch {
                source: store
                key: "UseNativeOpenAL"
                enabled: workarounds.overriding
                label: i18n("Use the system's OpenAL")
            }
        }

        SettingsGroup {
            width: parent.width
            title: i18n("Data packs")
            description: i18n("A folder of data packs added to every world. Needs a loader mod such as Paxi or OpenLoader.")
            SettingSwitch {
                id: globalPacks
                source: store
                key: "GlobalDataPacksEnabled"
                label: i18n("Use a shared data pack folder")
            }
            SettingText {
                source: store
                key: "GlobalDataPacksPath"
                enabled: globalPacks.checked
                label: i18n("Folder, inside the game folder")
                placeholder: "datapacks"
            }
        }

        PluginSurfaces {
            width: parent.width
            model: root.pluginSurfaces
        }

        SettingsGroup {
            width: parent.width
            title: i18n("On launch")
            SettingSwitch {
                id: joinServer
                source: store
                key: "JoinServerOnLaunch"
                label: i18n("Join a server when the game starts")
            }
            SettingText {
                source: store
                key: "JoinServerOnLaunchAddress"
                enabled: joinServer.checked
                label: i18n("Server address")
                placeholder: "play.example.net"
            }
        }
    }
}
