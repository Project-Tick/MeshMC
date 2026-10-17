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
    // Bumped after a choice that changes what the options say (the
    // profiler), so they are asked again.
    property int optionsRevision: 0
    // supportsDemo, accounts, profiler, profilers ... (QmlShell::launchOptions()).
    readonly property var options: {
        actions.optionsRevision
        return actions.instanceId.length > 0 ? actions.app.launchOptions(actions.instanceId) : ({})
    }
    // [{id, name, isDefault}]: Play As is only a choice with two or more.
    readonly property var accounts: actions.options.accounts || []
    // [{key, name, ready}], and the key in use ("" for none).
    readonly property var profilers: actions.options.profilers || []
    readonly property string profiler: actions.options.profiler || ""

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

    // The entries of Play As, one per account.
    readonly property Instantiator accountEntries: Instantiator {
        model: actions.accounts
        delegate: Kirigami.Action {
            required property var modelData
            text: modelData.isDefault ? i18nc("@action:inmenu %1 is an account name", "%1 (default)", modelData.name)
                                      : modelData.name
            // The account's own head, in its own colours.
            icon.source: "image://accountface/" + modelData.id
            icon.color: "transparent"
            onTriggered: actions.playAsAccount(modelData.id)
        }
        onObjectAdded: actions.syncChildren(playAsAction, actions.accountEntries)
        onObjectRemoved: actions.syncChildren(playAsAction, actions.accountEntries)
    }
    function playAsAccount(accountId) {
        if (actions.app.sendToAccountsIfNone())
            return
        if (actions.app.shell && typeof actions.app.shell.launchInstanceAs === "function")
            actions.app.shell.launchInstanceAs(actions.instanceId, accountId)
    }

    // The entries of Profiler: None, then each one the launcher knows.
    readonly property Instantiator profilerEntries: Instantiator {
        model: [{ key: "", name: i18nc("@item:inmenu no profiler", "None"), ready: true }].concat(actions.profilers)
        delegate: Kirigami.Action {
            required property var modelData
            // One that is not set up yet leads to where it is set up
            // instead of being a dead, greyed-out entry.
            text: modelData.ready ? modelData.name : i18nc("@action:inmenu %1 is a profiler", "Set Up %1…", modelData.name)
            checkable: true
            checked: modelData.ready && actions.profiler === modelData.key
            onTriggered: {
                if (modelData.ready)
                    actions.chooseProfiler(modelData.key)
                else
                    actions.app.navigate("settings/tools")
                checked = Qt.binding(() => modelData.ready && actions.profiler === modelData.key)
            }
        }
        onObjectAdded: actions.syncChildren(profilerAction, actions.profilerEntries)
        onObjectRemoved: actions.syncChildren(profilerAction, actions.profilerEntries)
    }
    function chooseProfiler(key) {
        if (actions.app.shell && typeof actions.app.shell.setInstanceProfiler === "function")
            actions.app.shell.setInstanceProfiler(actions.instanceId, key)
        actions.optionsRevision++
    }
    // Makes the actions an Instantiator built the sub-actions of @p owner.
    function syncChildren(owner, maker) {
        var list = []
        for (var i = 0; i < maker.count; ++i)
            list.push(maker.objectAt(i))
        owner.children = list
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
        // Play with another account than the default, without making it the
        // default: one entry per account, with its face.
        Kirigami.Action {
            id: playAsAction
            text: i18nc("@action:inmenu", "Play As")
            icon.name: "user-identity"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            visible: !actions.running && actions.accounts.length > 1
            enabled: actions.canLaunch
        },
        // Which profiler watches the game. It is a setting of the instance,
        // not of one launch: every later Play uses it, until "None" is picked.
        Kirigami.Action {
            id: profilerAction
            text: i18nc("@action:inmenu", "Profiler")
            icon.name: "utilities-system-monitor"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            visible: !actions.running && actions.profilers.length > 0
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
    readonly property int playEntries: 5

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
