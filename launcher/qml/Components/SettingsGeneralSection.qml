// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls

/*
 * The "General" section of SettingsPage.qml: how the library sorts, what a
 * new instance starts out with, where MeshMC keeps its folders (and which
 * other folders it looks in for instances), and the news notices.
 */
SettingsScroll {
    id: root

    // Show a folder in the file manager; SettingsPage passes it up to Main.qml.
    signal openPathRequested(string path)

    title: i18n("General")
    description: i18n("How the library sorts, where new instances start out, and where MeshMC keeps its folders.")
    SettingsGroup {
        width: parent.width
        title: i18n("Library")
        SettingChoice {
            key: "InstSortMode"
            label: i18n("Sort instances by")
            description: i18n("Order inside each group of the library.")
            options: [
                { value: "Name", label: i18n("Name") },
                { value: "LastLaunch", label: i18n("Last played") }
            ]
        }
    }
    SettingsGroup {
        width: parent.width
        title: i18n("New instances")
        SettingSwitch {
            key: "DownloadGameFilesDuringInstanceCreation"
            label: i18n("Download game files right away")
            description: i18n("Fetch libraries and assets while the instance is created, instead of on its first launch.")
        }
        SettingSwitch {
            key: "SkipModpackUpdatePrompt"
            invert: true
            label: i18n("Offer to update an installed modpack")
            description: i18n("Installing a modpack you already have suggests updating that instance instead of making a second copy.")
        }
        SettingSwitch {
            key: "BackupBeforeLaunch"
            label: i18n("Back up before every launch")
            description: i18n("Saves a backup of the instance, shown in its Backups tab, just before the game starts.")
        }
    }
    SettingsGroup {
        width: parent.width
        title: i18n("Folders")
        description: i18n("Relative paths are inside MeshMC's data folder. Changing a folder does not move what is already in it.")
        Repeater {
            model: [
                { key: "InstanceDir", label: i18n("Instances") },
                { key: "CentralModsDir", label: i18n("Mods") },
                { key: "IconsDir", label: i18n("Icons") },
                { key: "SkinsDir", label: i18n("Skins") },
                { key: "JavaDir", label: i18n("Java runtimes") }
            ]
            delegate: FolderRow {
                required property var modelData
                key: modelData.key
                label: modelData.label
                onOpenRequested: (path) => root.openPathRequested(path)
            }
        }
    }
    ExtraInstanceFolders {
        width: parent.width
        onOpenRequested: (path) => root.openPathRequested(path)
    }
    SettingsGroup {
        width: parent.width
        title: i18n("Notifications")
        SettingRow {
            label: i18n("Show notices again")
            description: i18n("News and announcements you have already seen come back the next time MeshMC checks.")
            enabled: SettingsStore.string("ShownNotifications").length > 0
            Button {
                text: i18n("Reset")
                onClicked: SettingsStore.setValue("ShownNotifications", "")
            }
        }
    }
}
