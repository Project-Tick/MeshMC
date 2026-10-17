// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components

/*
 * The instance's components: Minecraft, a mod loader, LWJGL and whatever
 * else is installed beside them. The toolbar changes the Minecraft version
 * and installs a loader; its overflow menu holds the rarer actions --
 * reloading, an empty component, jar mods, a replacement Minecraft.jar,
 * downloading the game files again, and the libraries and game folders.
 * Each component's own actions (change version, customize, revert, edit
 * JSON, move, remove) are behind its "More" button and right-click.
 *
 * Long operations run through PackProfile's own task, shown in a footer
 * rather than a blocking dialog. Everything is locked while the game runs.
 */
Item {
    id: tab

    required property var app
    property var details: null

    readonly property var components: tab.details ? tab.details.components : null
    readonly property var installer: tab.details ? tab.details.loaderInstaller : null
    // ComponentTools: the actions in the overflow menu and "Edit JSON".
    readonly property var tools: tab.details ? tab.details.componentTools : null
    readonly property bool unlocked: !!tab.details && tab.details.contentChangesAllowed
    readonly property bool idle: tab.unlocked && !!tab.components && !tab.components.busy
    // The TaskWatcher of "Download All Game Files", while it runs and after
    // it fails.
    property var downloadWatcher: null
    readonly property bool downloading: !!tab.downloadWatcher && tab.downloadWatcher.running
    // Every loader uid the launcher can install: those rows can pick another
    // version (LWJGL and the like cannot).
    readonly property var loaderUids: tab.installer ? tab.installer.loaders.map(l => l.uid) : []

    // PackProfile reports failures in launcher terms; the one a player most
    // often meets (the version metadata could not be fetched or checked) is
    // said here in plain words. Anything else is shown as it comes.
    readonly property string unresolvedError: i18n("Instance has unresolved dependencies while loading/checking for launch.")
    function friendlyError(error) {
        return error === tab.unresolvedError
               ? i18n("Some of this instance's version files could not be checked. Check your connection, then reload.")
               : error
    }

    function openChangeVersion(uid) {
        if (uid === "net.minecraft") {
            minecraftVersion.open()
        } else if (tab.installer) {
            loaderInstall.preselectUid = uid
            loaderInstall.open()
        }
    }
    // For review snapshots (DevRoute.qml's "review=<name>").
    function review(name) {
        if (name === "minecraftversion")
            minecraftVersion.open()
        else if (name === "loader")
            tab.openChangeVersion("")
        else if (name === "newcomponent")
            newComponent.start()
        else
            return false
        return true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabToolBar {
            Layout.fillWidth: true
            actions: [
                Kirigami.Action {
                    text: i18nc("@action:button", "Change Minecraft Version…")
                    icon.name: "document-edit"
                    enabled: tab.idle
                    onTriggered: minecraftVersion.open()
                },
                Kirigami.Action {
                    text: i18nc("@action:button", "Install Loader…")
                    icon.name: "download"
                    enabled: tab.idle && !!tab.installer
                    onTriggered: {
                        loaderInstall.preselectUid = ""
                        loaderInstall.open()
                    }
                },
                Kirigami.Action {
                    text: i18nc("@action:inmenu", "Reload")
                    icon.name: "view-refresh"
                    displayHint: Kirigami.DisplayHint.AlwaysHide
                    enabled: tab.idle
                    onTriggered: tab.components.reloadProfile()
                },
                Kirigami.Action {
                    text: i18nc("@action:inmenu", "Add Empty Component…")
                    icon.name: "list-add"
                    displayHint: Kirigami.DisplayHint.AlwaysHide
                    visible: !!tab.tools
                    enabled: tab.idle
                    onTriggered: newComponent.start()
                },
                Kirigami.Action {
                    text: i18nc("@action:inmenu", "Add to Minecraft.jar…")
                    icon.name: "archive-insert"
                    displayHint: Kirigami.DisplayHint.AlwaysHide
                    visible: !!tab.tools
                    enabled: tab.idle
                    onTriggered: jarMods.open()
                },
                Kirigami.Action {
                    text: i18nc("@action:inmenu", "Replace Minecraft.jar…")
                    icon.name: "document-replace"
                    displayHint: Kirigami.DisplayHint.AlwaysHide
                    visible: !!tab.tools
                    enabled: tab.idle
                    onTriggered: mainJar.open()
                },
                Kirigami.Action {
                    text: i18nc("@action:inmenu", "Download All Game Files")
                    icon.name: "download"
                    displayHint: Kirigami.DisplayHint.AlwaysHide
                    visible: !!tab.tools
                    enabled: tab.unlocked && !tab.downloading
                    onTriggered: tab.downloadWatcher = tab.tools.downloadAll()
                },
                Kirigami.Action {
                    text: i18nc("@action:inmenu", "Open Libraries Folder")
                    icon.name: "document-open-folder"
                    displayHint: Kirigami.DisplayHint.AlwaysHide
                    visible: !!tab.tools
                    onTriggered: tab.tools.openLibrariesFolder()
                },
                Kirigami.Action {
                    text: i18nc("@action:inmenu", "Open Game Folder")
                    icon.name: "document-open-folder"
                    displayHint: Kirigami.DisplayHint.AlwaysHide
                    visible: !!tab.tools
                    onTriggered: tab.tools.openGameFolder()
                }
            ]
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: !tab.unlocked && !!tab.details
            position: Kirigami.InlineMessage.Header
            type: Kirigami.MessageType.Information
            text: i18n("The game is running. Components can be changed once it closes.")
        }
        // What went wrong while resolving; its own message comes back with
        // every new failure.
        Kirigami.InlineMessage {
            id: errorMessage
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            position: Kirigami.InlineMessage.Header
            type: Kirigami.MessageType.Error
            showCloseButton: true
            text: tab.components ? tab.friendlyError(tab.components.lastError) : ""
            actions: [
                Kirigami.Action {
                    text: i18nc("@action:button", "Reload")
                    icon.name: "view-refresh"
                    visible: !!tab.components && tab.components.lastError === tab.unresolvedError
                    enabled: tab.idle
                    onTriggered: {
                        errorMessage.visible = false
                        tab.components.reloadProfile()
                    }
                }
            ]
            function show() {
                visible = Qt.binding(() => !!tab.components && tab.components.lastError.length > 0)
            }
            Component.onCompleted: show()
            Connections {
                target: tab.components
                function onLastErrorChanged() {
                    errorMessage.show()
                }
            }
        }
        // "Download All Game Files" failing, or a tool saying why not.
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: text.length > 0
            position: Kirigami.InlineMessage.Header
            type: Kirigami.MessageType.Warning
            text: !!tab.downloadWatcher && tab.downloadWatcher.failed
                  ? (tab.downloadWatcher.error.length > 0 ? tab.downloadWatcher.error : i18n("Downloading the game files failed."))
                  : (tab.tools ? tab.tools.error : "")
        }

        ListArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                clip: true
                currentIndex: -1
                model: tab.components
                delegate: ComponentRow {
                    id: row
                    width: ListView.view.width
                    unlocked: tab.unlocked
                    readonly property string uid: row.model.uid
                    readonly property bool changeable: row.uid === "net.minecraft" || tab.loaderUids.indexOf(row.uid) >= 0
                    hasActions: row.model.isMoveable === true || (row.changeable && row.model.hasVersionList === true)
                                || (row.model.isCustomizable === true && !row.model.isCustom)
                                || row.model.isRevertible === true || row.model.isRemovable === true
                    onToggleRequested: (uid, on) => tab.components.setComponentEnabled(uid, on)
                    onMenuRequested: (parent, position) => tab.openMenu({
                        index: row.index,
                        uid: row.uid,
                        name: row.model.name,
                        changeable: row.changeable && row.model.hasVersionList === true,
                        customizable: row.model.isCustomizable === true && !row.model.isCustom,
                        revertible: row.model.isRevertible === true,
                        moveable: row.model.isMoveable === true,
                        removable: row.model.isRemovable === true,
                        hasFile: !!tab.tools && tab.tools.hasJsonFile(row.index)
                    }, parent, position)
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 4
                    visible: list.count === 0
                    icon.name: "package-x-generic"
                    text: i18n("No components")
                    explanation: i18n("This instance has nothing installed yet.")
                }
            }
        }

        // The profile updating, or the game files downloading.
        ProgressFooter {
            Layout.fillWidth: true
            readonly property bool profileBusy: !!tab.components && tab.components.busy
            active: profileBusy || tab.downloading
            status: profileBusy
                    ? (tab.components.task
                       ? (tab.components.task.status.length > 0 ? tab.components.task.status
                          : (tab.components.task.title.length > 0 ? tab.components.task.title : i18n("Updating…")))
                       : i18n("Updating…"))
                    : (tab.downloading ? (tab.downloadWatcher.status.length > 0 ? tab.downloadWatcher.status
                                                                                : i18n("Downloading game files…")) : "")
            progress: profileBusy ? (tab.components.task ? tab.components.task.progress : -1)
                                  : (tab.downloading ? tab.downloadWatcher.progress : -1)
        }
    }

    // One component's menu.
    function openMenu(entry, parent, position) {
        menuComponent.createObject(tab, { entry: entry }).popup(parent, position)
    }
    Component {
        id: menuComponent
        Components.ConvergentContextMenu {
            id: menu
            required property var entry
            onClosed: menu.destroy()
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Change Version…")
                icon.name: "document-edit"
                visible: menu.entry.changeable
                enabled: tab.idle
                onTriggered: tab.openChangeVersion(menu.entry.uid)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Customize")
                icon.name: "document-edit"
                visible: menu.entry.customizable
                enabled: tab.idle
                onTriggered: tab.components.customizeComponent(menu.entry.index)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Revert to the Original…")
                icon.name: "document-revert"
                visible: menu.entry.revertible
                enabled: tab.idle
                onTriggered: {
                    revertDialog.row = menu.entry.index
                    revertDialog.subtitle = i18n("Revert “%1” to its original version? Your own changes to it are lost.", menu.entry.name)
                    revertDialog.open()
                }
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Edit JSON")
                icon.name: "text-x-script"
                visible: menu.entry.hasFile
                onTriggered: tab.tools.editComponentJson(menu.entry.index)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Move Up")
                icon.name: "go-up"
                visible: menu.entry.moveable
                enabled: tab.idle && menu.entry.index > 0
                onTriggered: tab.components.moveComponentUp(menu.entry.index)
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Move Down")
                icon.name: "go-down"
                visible: menu.entry.moveable
                enabled: tab.idle && menu.entry.index < list.count - 1
                onTriggered: tab.components.moveComponentDown(menu.entry.index)
            }
            Kirigami.Action {
                separator: true
                visible: menu.entry.removable
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "Remove…")
                icon.name: "edit-delete"
                visible: menu.entry.removable
                enabled: tab.idle
                onTriggered: {
                    removeDialog.row = menu.entry.index
                    removeDialog.subtitle = i18n("Remove “%1” from this instance?", menu.entry.name)
                    removeDialog.open()
                }
            }
        }
    }

    ConfirmActionDialog {
        id: removeDialog
        property int row: -1
        title: i18nc("@title:dialog", "Remove Component")
        onConfirmed: if (tab.components) tab.components.removeComponent(removeDialog.row)
    }
    ConfirmActionDialog {
        id: revertDialog
        property int row: -1
        title: i18nc("@title:dialog", "Revert Component")
        actionText: i18nc("@action:button", "Revert")
        actionIcon: "document-revert"
        onConfirmed: if (tab.components) tab.components.revertComponent(revertDialog.row)
    }

    MinecraftVersionDialog {
        id: minecraftVersion
        details: tab.details
    }
    LoaderInstallDialog {
        id: loaderInstall
        installer: tab.installer
    }
    NewComponentDialog {
        id: newComponent
        tools: tab.tools
    }

    Dialogs.FileDialog {
        id: jarMods
        title: i18nc("@title:dialog", "Choose Jar Mods")
        fileMode: Dialogs.FileDialog.OpenFiles
        nameFilters: [i18n("Minecraft.jar mods (*.zip *.jar)")]
        onAccepted: tab.tools.addJarMods(selectedFiles)
    }
    Dialogs.FileDialog {
        id: mainJar
        title: i18nc("@title:dialog", "Choose the Replacement Minecraft.jar")
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: [i18n("Minecraft.jar replacement (*.jar)")]
        onAccepted: tab.tools.replaceMainJar(selectedFile)
    }
}
