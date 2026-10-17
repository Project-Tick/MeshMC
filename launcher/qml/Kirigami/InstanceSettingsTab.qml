// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * One instance's own settings (details.settings, a SettingsAdapter over the
 * instance's settings): each card follows Settings until switched to
 * custom for this instance (OverridableCard.qml) -- memory and Java, the
 * game window, the console, custom commands, play time, native libraries --
 * then its shared data pack folder, what plugins add, and the server it
 * joins on launch.
 * Locked while the game runs: it has already read them, and a change now
 * would do nothing until the next launch.
 */
QQC2.ScrollView {
    id: tab

    required property var app
    property var details: null
    property bool running: false

    readonly property SettingsSource store: SettingsSource {
        adapter: tab.details && tab.details.settings ? tab.details.settings : null
    }

    contentWidth: availableWidth
    // FormCardPage's background: between the window's and a view's.
    background: Rectangle {
        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        Kirigami.Theme.inherit: false
        Item {
            id: viewColors
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false
        }
        color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor,
                                                       viewColors.Kirigami.Theme.backgroundColor, 0.5)
    }

    ColumnLayout {
        width: tab.availableWidth
        spacing: 0

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.largeSpacing
            visible: tab.running
            text: i18n("The game is running. Its settings can be changed once it has closed.")
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: Kirigami.Units.gridUnit
            spacing: 0
            enabled: !tab.running

            InstanceJavaCards {
                app: tab.app
                source: tab.store
            }

            FormCard.FormHeader {
                title: i18nc("@title:group", "Game Window")
            }
            GameWindowCard {
                source: tab.store
                overrideKey: "OverrideWindow"
            }

            FormCard.FormHeader {
                title: i18nc("@title:group", "Console")
            }
            ConsoleCard {
                source: tab.store
                overrideKey: "OverrideConsole"
            }

            FormCard.FormHeader {
                title: i18nc("@title:group", "Custom Commands")
            }
            CommandsCard {
                source: tab.store
                overrideKey: "OverrideCommands"
            }

            FormCard.FormHeader {
                title: i18nc("@title:group", "Play Time")
            }
            OverridableCard {
                id: playTime
                source: tab.store
                overrideKey: "OverrideGameTime"
                overrideKeys: ["RecordGameTime", "ShowGameTime"]
                SettingSwitch {
                    source: tab.store
                    key: "RecordGameTime"
                    enabled: playTime.editable
                    text: i18nc("@option:check", "Record play time")
                }
                FormCard.FormDelegateSeparator {}
                SettingSwitch {
                    source: tab.store
                    key: "ShowGameTime"
                    enabled: playTime.editable
                    text: i18nc("@option:check", "Show play time")
                }
            }

            FormCard.FormHeader {
                title: i18nc("@title:group", "Native Libraries")
            }
            NativeLibrariesCard {
                source: tab.store
                overrideKey: "OverrideNativeWorkarounds"
                overrideText: i18nc("@option:check", "Use custom native library settings for this instance")
            }

            FormCard.FormHeader {
                title: i18nc("@title:group", "Data Packs")
            }
            FormCard.FormCard {
                SettingSwitch {
                    id: globalPacks
                    source: tab.store
                    key: "GlobalDataPacksEnabled"
                    text: i18nc("@option:check", "Use a shared data pack folder")
                    description: i18n("Data packs in it are added to every world. Needs a loader mod such as Paxi or OpenLoader.")
                }
                FormCard.FormDelegateSeparator {}
                SettingText {
                    source: tab.store
                    key: "GlobalDataPacksPath"
                    enabled: globalPacks.checked
                    label: i18nc("@label:textbox", "Folder, inside the game folder")
                    placeholderText: "datapacks"
                }
            }

            // What plugins add to an instance's settings.
            PluginSurfaces {
                model: tab.app.shell && typeof tab.app.shell.pluginSurfaces === "function" && tab.details
                       ? tab.app.shell.pluginSurfaces(2, tab.details.instanceId) : null
            }

            FormCard.FormHeader {
                title: i18nc("@title:group", "On Launch")
            }
            FormCard.FormCard {
                SettingSwitch {
                    id: joinServer
                    source: tab.store
                    key: "JoinServerOnLaunch"
                    text: i18nc("@option:check", "Join a server when the game starts")
                }
                FormCard.FormDelegateSeparator {}
                SettingText {
                    source: tab.store
                    key: "JoinServerOnLaunchAddress"
                    enabled: joinServer.checked
                    label: i18nc("@label:textbox", "Server address")
                    placeholderText: "play.example.net"
                }
            }
        }
    }
}
