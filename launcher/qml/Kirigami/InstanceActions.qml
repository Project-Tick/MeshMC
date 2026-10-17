// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * Everything that can be done to one instance, as Kirigami actions: the
 * library's context menu shows them, and the instance page puts Play and
 * Open Folder in its toolbar with the rest in its overflow menu. Defined
 * once here so both say and do exactly the same.
 *
 * Set the instance's id, name, icon, group and whether it runs; the actions
 * call into `app` (Main.qml) and its `dialogs`.
 */
QtObject {
    id: actions

    required property var app
    property string instanceId
    property string name
    property string iconKey
    property string group
    property bool running: false
    property bool canLaunch: true

    // {instance, game, mods}: `game` only when it is not the instance
    // folder itself, `mods` only for Minecraft (LibraryController.folders()).
    readonly property var folders: actions.app.shell && actions.app.shell.library && actions.instanceId.length > 0
                                   ? actions.app.shell.library.folders(actions.instanceId) : ({})
    readonly property bool hasGameFolder: !!actions.folders.game && actions.folders.game !== actions.folders.instance
    // supportsDemo, accounts, ... (QmlShell::launchOptions()).
    readonly property var options: actions.instanceId.length > 0 ? actions.app.launchOptions(actions.instanceId) : ({})

    function openPath(path) {
        if (actions.app.shell && actions.app.shell.library)
            actions.app.shell.library.openFolder(path)
    }

    readonly property Kirigami.Action play: Kirigami.Action {
        text: actions.running ? i18nc("@action:button", "Stop") : i18nc("@action:button", "Play")
        icon.name: actions.running ? "media-playback-stop" : "media-playback-start"
        enabled: actions.running || actions.canLaunch
        onTriggered: actions.running ? actions.app.stop(actions.instanceId) : actions.app.launch(actions.instanceId)
    }
    readonly property Kirigami.Action open: Kirigami.Action {
        text: i18nc("@action:inmenu", "Open")
        icon.name: "go-next"
        onTriggered: actions.app.openInstance(actions.instanceId)
    }
    readonly property Kirigami.Action openFolder: Kirigami.Action {
        text: i18nc("@action:button", "Open Folder")
        icon.name: "document-open-folder"
        onTriggered: actions.app.call("openInstanceFolder", actions.instanceId)
    }

    // The rest, in the order a menu lists them. AlwaysHide keeps them in the
    // instance page's overflow menu.
    readonly property list<QtObject> more: [
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Play Offline")
            icon.name: "network-offline"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            visible: !actions.running
            enabled: actions.canLaunch
            onTriggered: actions.app.launchMode(actions.instanceId, "offline")
        },
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Play Demo")
            icon.name: "media-playback-start"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            visible: !actions.running && actions.options.supportsDemo === true
            enabled: actions.canLaunch
            onTriggered: actions.app.launchMode(actions.instanceId, "demo")
        },
        Kirigami.Action {
            separator: true
            displayHint: Kirigami.DisplayHint.AlwaysHide
            visible: !actions.running
        },
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Open .minecraft Folder")
            icon.name: "document-open-folder"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            visible: actions.hasGameFolder
            onTriggered: actions.openPath(actions.folders.game)
        },
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Open Mods Folder")
            icon.name: "document-open-folder"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            visible: !!actions.folders.mods
            onTriggered: actions.openPath(actions.folders.mods)
        },
        Kirigami.Action {
            separator: true
            displayHint: Kirigami.DisplayHint.AlwaysHide
        },
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Rename…")
            icon.name: "edit-rename"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: actions.app.dialogs.askRename(actions.instanceId, actions.name)
        },
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Change Icon…")
            icon.name: "preferences-desktop-icons"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: actions.app.dialogs.askIcon(actions.instanceId, actions.iconKey)
        },
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Change Group…")
            icon.name: "tag"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: actions.app.dialogs.askGroup(actions.instanceId, actions.group)
        },
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Duplicate…")
            icon.name: "edit-copy"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: actions.app.dialogs.duplicate(actions.instanceId, actions.name, actions.group)
        },
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Export…")
            icon.name: "document-export"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: actions.app.dialogs.exportInstance(actions.instanceId)
        },
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Create Shortcut…")
            icon.name: "link"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: actions.app.dialogs.createShortcut(actions.instanceId)
        },
        Kirigami.Action {
            separator: true
            displayHint: Kirigami.DisplayHint.AlwaysHide
        },
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Delete…")
            icon.name: "edit-delete"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            // The game has the folder open.
            enabled: !actions.running
            onTriggered: actions.app.dialogs.askDelete(actions.instanceId, actions.name)
        }
    ]

    // How many of `more` are ways to play, with the separator after them.
    readonly property int playEntries: 3

    // For the library's context menu: the ways to play, then Open and the
    // folders, then the rest.
    function menuActions() {
        var list = [actions.play]
        for (var i = 0; i < actions.more.length; ++i) {
            if (i === actions.playEntries)
                list.push(actions.open, actions.openFolder)
            list.push(actions.more[i])
        }
        return list
    }
}
