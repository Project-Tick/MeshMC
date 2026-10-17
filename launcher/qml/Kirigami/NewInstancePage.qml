// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * "New Instance…": a Minecraft version and, if wanted, a mod loader and its
 * build, under a name, a group and an icon. The work is
 * NewInstanceController's (shell.newInstance), the same the classic window
 * used. Create shows how it is going in the footer, then opens the new
 * instance. Leaving the page does not stop it: the window says when it is
 * ready instead (Main.qml's trackModpackInstall()).
 *
 * The name follows the version and loader ("Minecraft 1.21.4", "1.21.4
 * Fabric") until one is typed.
 */
FormCard.FormCardPage {
    id: page

    required property var app
    readonly property var controller: page.app.shell && page.app.shell.newInstance ? page.app.shell.newInstance : null
    readonly property var versions: page.controller ? page.controller.minecraftVersions : null

    // This page's creation, as the window keeps it (Main.qml's
    // modpackInstalls): the window stays quiet about it while this page is
    // on screen to say how it went itself.
    readonly property string installKey: "new/" + page.openedAt
    readonly property double openedAt: Date.now()
    property var watcher: null
    readonly property bool creating: !!page.watcher && page.watcher.running
    readonly property bool canCreate: !page.creating && !!page.controller
                                      && page.controller.selectedMinecraftVersion.length > 0
                                      && identity.name.length > 0

    // A vanilla instance is named after its bare version ("26.3"), which
    // reads as a number in the library: it gets the game's name in front.
    // A modded one ("26.3 Fabric") already says what it is.
    function suggestName() {
        if (!page.controller)
            return
        var suggested = page.controller.suggestedName()
        if (suggested.length > 0 && page.controller.loader.length === 0)
            suggested = i18nc("@info instance name, %1 Minecraft version", "Minecraft %1", suggested)
        identity.suggest(suggested)
    }
    // The newest version of the list shown, so Create works straight away;
    // a version already picked stays.
    function selectDefaultVersion() {
        if (!page.versions || page.controller.selectedMinecraftVersion.length > 0)
            return
        if (page.versions.firstVersionId.length > 0)
            page.controller.selectMinecraftVersion(page.versions.firstVersionId)
    }

    function create() {
        if (!page.canCreate)
            return
        var watcher = page.controller.create(identity.name, identity.group, identity.iconKey)
        if (!watcher) {
            page.app.notify(i18n("This version can't be installed. Try reloading the version list."))
            return
        }
        page.watcher = watcher
        page.app.trackModpackInstall(page.installKey, watcher, identity.name)
    }

    // For review snapshots (DevRoute.qml): "version=<n>" picks row n,
    // "loader=<id>" a loader, "install" presses Create once it can.
    function pickVersion(n) {
        var version = versionList.versionAt(n)
        if (version.length === 0)
            return false
        page.controller.selectMinecraftVersion(version)
        return true
    }
    function pickLoader(id) {
        loaderCard.pick(id)
        return true
    }
    function installWhenReady() {
        if (!page.canCreate)
            return page.creating
        page.create()
        return true
    }

    // How Navigator finds this page on top of the stack.
    objectName: "newInstancePage"
    title: i18nc("@title", "New Instance")

    actions: [
        Kirigami.Action {
            text: page.creating ? i18nc("@action:button", "Creating…") : i18nc("@action:button", "Create")
            icon.name: "list-add"
            enabled: page.canCreate
            onTriggered: page.create()
        }
    ]

    footer: ProgressFooter {
        active: page.creating
        status: page.watcher && page.watcher.status.length > 0 ? page.watcher.status
                                                              : i18nc("@info:status", "Creating…")
        progress: page.watcher ? page.watcher.progress : -1
    }

    Component.onCompleted: {
        page.selectDefaultVersion()
        page.suggestName()
        identity.focusName()
    }
    Connections {
        target: page.versions
        ignoreUnknownSignals: true
        function onFirstVersionIdChanged() {
            page.selectDefaultVersion()
        }
    }
    Connections {
        target: page.controller
        ignoreUnknownSignals: true
        function onSelectedMinecraftVersionChanged() {
            page.suggestName()
        }
        function onLoaderChanged() {
            page.suggestName()
        }
    }
    Connections {
        target: page.watcher
        function onFinished(ok) {
            if (!ok)
                return
            var id = page.watcher.instanceId
            if (!id || !page.app.openInstance(id)) {
                page.app.showSection("library")
                page.app.notify(i18n("“%1” is in your library.", page.watcher.title))
            }
        }
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 30
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Kirigami.Units.largeSpacing
        visible: !!page.watcher && page.watcher.failed
        type: Kirigami.MessageType.Error
        text: page.watcher && page.watcher.error.length > 0 ? page.watcher.error
                                                           : i18n("The instance could not be created.")
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 30
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Kirigami.Units.largeSpacing
        visible: !page.controller
        type: Kirigami.MessageType.Warning
        text: i18n("New instances cannot be made right now.")
    }

    InstanceIdentityCard {
        id: identity
        Layout.topMargin: Kirigami.Units.largeSpacing * 2
        app: page.app
        enabled: !page.creating
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Minecraft Version")
        actions: [
            Kirigami.Action {
                text: i18nc("@option:check", "Snapshots")
                checkable: true
                checked: !!page.versions && page.versions.showSnapshots
                enabled: !!page.versions && !page.creating
                onToggled: page.versions.showSnapshots = checked
            },
            Kirigami.Action {
                text: i18nc("@option:check", "Old Versions")
                checkable: true
                checked: !!page.versions && page.versions.showOldVersions
                enabled: !!page.versions && !page.creating
                onToggled: page.versions.showOldVersions = checked
            }
        ]
    }
    FormCard.FormCard {
        MinecraftVersionList {
            id: versionList
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 14
            controller: page.controller
            enabled: !page.creating
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Mod Loader")
    }
    LoaderPickerCard {
        id: loaderCard
        Layout.bottomMargin: Kirigami.Units.largeSpacing * 2
        controller: page.controller
        enabled: !page.creating
    }
}
