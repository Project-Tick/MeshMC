// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * General: how the library sorts, what a new instance starts out with,
 * where MeshMC keeps its folders and which other folders it finds
 * instances in, and the news notices.
 */
SettingsPage {
    id: page

    readonly property var folders: page.shell && page.shell.folderSettings ? page.shell.folderSettings : null
    readonly property var extraDirs: page.folders ? page.folders.additionalDirs : []

    function openPath(path) {
        page.app.call("openPath", path)
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Library")
    }
    FormCard.FormCard {
        SettingChoice {
            source: page.store
            key: "InstSortMode"
            text: i18nc("@label:listbox", "Sort instances by")
            description: i18n("The order inside each group of the library.")
            options: [
                { value: "Name", label: i18nc("@item:inlistbox sort by", "Name") },
                { value: "LastLaunch", label: i18nc("@item:inlistbox sort by", "Last played") }
            ]
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "New Instances")
    }
    FormCard.FormCard {
        SettingSwitch {
            source: page.store
            key: "DownloadGameFilesDuringInstanceCreation"
            text: i18nc("@option:check", "Download game files right away")
            description: i18n("Fetch libraries and assets while the instance is made, instead of on its first launch.")
        }
        FormCard.FormDelegateSeparator {}
        SettingSwitch {
            source: page.store
            key: "SkipModpackUpdatePrompt"
            invert: true
            text: i18nc("@option:check", "Offer to update an installed modpack")
            description: i18n("Installing a modpack you already have suggests updating that instance instead of making a second copy.")
        }
        FormCard.FormDelegateSeparator {}
        SettingSwitch {
            source: page.store
            key: "BackupBeforeLaunch"
            text: i18nc("@option:check", "Back up before every launch")
            description: i18n("Saves a backup of the instance just before the game starts.")
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Folders")
    }
    FormCard.FormCard {
        autoSeparators: true
        Repeater {
            model: [
                { key: "InstanceDir", label: i18nc("@label folder", "Instances") },
                { key: "CentralModsDir", label: i18nc("@label folder", "Mods") },
                { key: "IconsDir", label: i18nc("@label folder", "Icons") },
                { key: "SkinsDir", label: i18nc("@label folder", "Skins") },
                { key: "JavaDir", label: i18nc("@label folder", "Java runtimes") }
            ]
            delegate: FolderDelegate {
                required property var modelData
                source: page.store
                key: modelData.key
                text: modelData.label
                folders: page.folders
                onOpenRequested: (path) => page.openPath(path)
            }
        }
    }
    FormCard.FormSectionText {
        text: i18n("Relative paths are inside MeshMC's data folder. Changing a folder does not move what is already in it.")
    }

    FormCard.FormHeader {
        visible: !!page.folders
        title: i18nc("@title:group", "More Instance Folders")
        actions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Add Folder…")
                icon.name: "list-add"
                onTriggered: extraDialog.open()
            }
        ]
    }
    FormCard.FormCard {
        visible: !!page.folders
        autoSeparators: true
        FormCard.FormTextDelegate {
            visible: page.extraDirs.length === 0
            text: i18n("No other folders")
            description: i18n("Instances in folders added here show up in the library too. New instances are still made in the main folder.")
        }
        Repeater {
            model: page.extraDirs
            delegate: FormCard.FormTextDelegate {
                id: extra
                required property string modelData
                text: extra.modelData
                textItem.elide: Text.ElideMiddle
                trailing: RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    QQC2.Button {
                        icon.name: "document-open-folder"
                        text: i18nc("@action:button", "Open Folder")
                        display: QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.text: text
                        QQC2.ToolTip.visible: hovered
                        onClicked: page.openPath(extra.modelData)
                    }
                    QQC2.Button {
                        icon.name: "list-remove"
                        text: i18nc("@action:button", "Stop Looking Here")
                        display: QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.text: text
                        QQC2.ToolTip.visible: hovered
                        onClicked: page.folders.removeAdditionalDir(extra.modelData)
                    }
                }
            }
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Notifications")
    }
    FormCard.FormCard {
        FormCard.FormButtonDelegate {
            text: i18nc("@action:button", "Show notices again")
            description: i18n("News and announcements you have already seen come back the next time MeshMC checks.")
            icon.name: "view-refresh"
            enabled: page.store.string("ShownNotifications").length > 0
            onClicked: {
                page.store.setValue("ShownNotifications", "")
                page.notify(i18n("Notices you have seen will show again."))
            }
        }
    }

    Dialogs.FolderDialog {
        id: extraDialog
        title: i18nc("@title:window", "Add an Instance Folder")
        onAccepted: page.folders.addAdditionalDir(selectedFolder)
    }
}
