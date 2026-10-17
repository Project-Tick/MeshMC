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
 * Where this instance's modpack came from, and what other versions of it
 * exist: the installed version, a drop-down of the others with an Update
 * button, and -- once there is a version to read about -- its changelog.
 * "Update From File" works for any pack, including one the launcher has no
 * catalogue id for. Only shown for an instance installed from Modrinth or
 * CurseForge.
 */
Item {
    id: tab

    required property var app
    property var details: null

    // ManagedPackController, or null.
    readonly property var controller: tab.details ? tab.details.managedPack : null
    // The TaskWatcher of an update in progress, if any.
    property var watcher: null
    readonly property bool updating: !!tab.watcher && tab.watcher.running

    readonly property var versions: tab.controller ? tab.controller.versions : []
    readonly property bool hasVersions: !!tab.controller && tab.controller.hasPackId && tab.versions.length > 0
    // The version the drop-down shows, or null.
    readonly property var picked: tab.hasVersions && versionBox.currentIndex >= 0
                                  && versionBox.currentIndex < tab.versions.length
                                  ? tab.versions[versionBox.currentIndex] : null
    // One line per version: its name, the Minecraft it is for, and a word
    // where it is not a plain release or is the installed one.
    readonly property var versionLabels: tab.versions.map(v => {
        var label = String(v.name || v.label)
        if (v.minecraft)
            label = i18n("%1, Minecraft %2", label, v.minecraft)
        if (v.releaseType === "alpha")
            label = i18n("%1, alpha", label)
        else if (v.releaseType === "beta")
            label = i18n("%1, beta", label)
        return v.current ? i18n("%1 (installed)", label) : label
    })

    // Emitted once an update has landed on disk: this instance has just
    // been replaced, so the page showing it is no longer worth keeping.
    signal updated()

    // Fires at creation and again whenever another instance is opened.
    onControllerChanged: {
        tab.watcher = null
        if (tab.controller)
            tab.controller.fetchVersions()
    }

    // The lowercase URI scheme of @p s ("https" for "https://x"), or null.
    function schemeOf(s) {
        var m = /^([a-zA-Z][a-zA-Z0-9+.-]*):/.exec(String(s))
        return m ? m[1].toLowerCase() : null
    }

    // A changelog is remote, untrusted text, and handing an arbitrary scheme
    // to the desktop is how a "file:" or worse ends up opened, so only
    // http(s) links are passed on. A schemeless link is CurseForge's
    // outbound redirect, which in changelog HTML arrives as the relative
    // "linkout?remoteUrl=<percent-encoded>": the real destination is in the
    // query, decoded and scheme-checked the same way.
    function openChangelogLink(link) {
        var url = String(link)
        var scheme = tab.schemeOf(url)
        if (scheme !== null) {
            if (scheme === "http" || scheme === "https")
                Qt.openUrlExternally(url)
            else
                console.warn("ManagedPackTab: refusing to open changelog link with scheme:", scheme)
            return
        }

        var queryIndex = url.indexOf("?")
        var remote = ""
        if (queryIndex >= 0) {
            var parts = url.substring(queryIndex + 1).split("&")
            for (var i = 0; i < parts.length; ++i) {
                var eq = parts[i].indexOf("=")
                var key = eq >= 0 ? parts[i].substring(0, eq) : parts[i]
                if (key === "remoteUrl") {
                    var value = eq >= 0 ? parts[i].substring(eq + 1) : ""
                    try {
                        remote = decodeURIComponent(decodeURIComponent(value))
                    } catch (e) {
                        remote = ""
                    }
                    break
                }
            }
        }

        var remoteScheme = tab.schemeOf(remote)
        if (remoteScheme === "http" || remoteScheme === "https")
            Qt.openUrlExternally(remote)
        else if (remote.length > 0)
            console.warn("ManagedPackTab: refusing to open changelog redirect with scheme:", remoteScheme)
    }

    Connections {
        target: tab.watcher
        ignoreUnknownSignals: true
        function onFinished(ok) {
            if (ok)
                tab.updated()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: !!tab.controller

        FormTab {
            Layout.fillWidth: true
            Layout.fillHeight: true

            WatcherMessage {
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 30
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.largeSpacing
                watcher: tab.watcher
                failText: i18n("The update failed.")
            }

            FormCard.FormHeader {
                title: tab.controller ? i18nc("@title:group %1 is Modrinth or CurseForge", "%1 modpack", tab.controller.providerLabel) : ""
            }
            FormCard.FormCard {
                FormCard.FormTextDelegate {
                    text: tab.controller && tab.controller.packName.length > 0 ? tab.controller.packName : i18n("Modpack")
                    description: tab.controller && tab.controller.installedVersionName.length > 0
                                 ? i18n("Installed version: %1", tab.controller.installedVersionName)
                                 : i18n("Installed version unknown")
                }
                FormCard.FormDelegateSeparator {
                    visible: websiteDelegate.visible
                }
                FormCard.FormButtonDelegate {
                    id: websiteDelegate
                    visible: !!tab.controller && tab.controller.packUrl.length > 0
                    text: i18nc("@action:button", "Open Website")
                    icon.name: "internet-services"
                    onClicked: Qt.openUrlExternally(tab.controller.packUrl)
                }
                FormCard.FormDelegateSeparator {}
                FormCard.FormButtonDelegate {
                    // An update from a pack file on this computer: works for
                    // any pack, including one with no catalogue id.
                    text: i18nc("@action:button", "Update From File…")
                    description: tab.controller && tab.controller.running
                                 ? i18n("The game is running. The pack can be updated once it closes.") : ""
                    icon.name: "document-import"
                    enabled: !tab.updating && !!tab.controller && !tab.controller.running
                    onClicked: packFile.open()
                }
            }

            Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 30
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.largeSpacing
                visible: !!tab.controller && !tab.controller.hasPackId
                type: Kirigami.MessageType.Information
                text: i18n("MeshMC does not have a catalogue id on record for this pack, so it cannot list other versions here. You can still update it from a pack file, or install the pack again from Discover for a specific version.")
            }
            // What went wrong in plain words only: the catalogue's own
            // message is a raw job string meant for the log.
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 30
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.largeSpacing
                visible: !!tab.controller && tab.controller.hasPackId && tab.controller.error.length > 0
                type: Kirigami.MessageType.Error
                text: i18n("Could not load the versions of this pack. Check your connection, then reload.")
                actions: [
                    Kirigami.Action {
                        text: i18nc("@action:button", "Reload")
                        icon.name: "view-refresh"
                        onTriggered: tab.controller.reload()
                    }
                ]
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 30
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.largeSpacing
                visible: !!tab.controller && tab.controller.hasPackId && tab.controller.error.length === 0
                         && !tab.controller.loading && tab.versions.length === 0
                type: Kirigami.MessageType.Information
                text: i18n("The catalogue lists no other versions of this pack.")
            }
            QQC2.BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.largeSpacing
                visible: !!tab.controller && tab.controller.loading
                running: visible
            }

            // The other versions: the newest is picked at first, so its
            // changelog is there to read before updating to it.
            FormCard.FormHeader {
                visible: tab.hasVersions
                title: i18nc("@title:group", "Update")
            }
            FormCard.FormCard {
                visible: tab.hasVersions
                FormCard.FormComboBoxDelegate {
                    id: versionBox
                    text: i18nc("@label:listbox", "Update to")
                    model: tab.versionLabels
                }
                FormCard.FormDelegateSeparator {}
                FormCard.FormButtonDelegate {
                    // The tab's one main action, live only while it would
                    // do something: a version other than the installed one.
                    text: i18nc("@action:button", "Update")
                    icon.name: "download"
                    enabled: !tab.updating && !!tab.picked && tab.picked.installable && !tab.picked.current
                    onClicked: tab.watcher = tab.controller.updateToVersion(versionBox.currentIndex)
                }
            }

            FormCard.FormHeader {
                visible: tab.hasVersions && !!tab.picked
                title: i18nc("@title:group", "Changelog")
            }
            FormCard.FormCard {
                Layout.bottomMargin: Kirigami.Units.gridUnit
                visible: tab.hasVersions && !!tab.picked
                padding: Kirigami.Units.largeSpacing

                // A changelog may link images; see RemoteRichText.
                RemoteRichText {
                    Layout.fillWidth: true
                    source: !tab.picked ? ""
                            : tab.picked.changelog.length > 0
                            ? tab.picked.changelog
                            : i18n("No changelog available for this version.")
                    onLinkActivated: (link) => tab.openChangelogLink(link)
                }
            }
        }

        ProgressFooter {
            Layout.fillWidth: true
            active: tab.updating
            status: tab.watcher ? (tab.watcher.status || i18n("Updating…")) : ""
            progress: tab.watcher ? tab.watcher.progress : -1
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: !tab.controller
        icon.name: "package-x-generic"
        text: i18n("Not a managed pack")
        explanation: i18n("This instance was not installed from a catalogue MeshMC recognises.")
    }

    Dialogs.FileDialog {
        id: packFile
        title: i18nc("@title:dialog", "Choose the Pack File")
        nameFilters: tab.controller ? [tab.controller.fileFilter] : []
        onAccepted: tab.watcher = tab.controller.updateFromFile(selectedFile)
    }
}
