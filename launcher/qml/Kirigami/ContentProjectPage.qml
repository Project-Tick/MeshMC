// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.coreaddons as KCoreAddons
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

import "format.js" as Format

/*
 * One project from Add Content, pushed over it: what it is, then every
 * version, newest first, the newest that runs on this instance picked --
 * versions for another Minecraft version or loader are greyed and say so.
 * Install, in the toolbar, installs the picked one: what it needs is worked
 * out first and listed to untick (InstallReviewDialog), and the download
 * shows in the footer. A version of something already installed replaces
 * it.
 *
 * `owner` is the Add Content page, for its browser and the instance's
 * ContentInstaller; `row` is the project's place in the browser's results
 * and `project` what its row showed.
 */
Kirigami.ScrollablePage {
    id: page

    required property var app
    property var owner: null
    property int row: -1
    property var project: ({})

    readonly property var installer: page.owner ? page.owner.installer : null
    readonly property var browser: page.owner ? page.owner.browser : null

    // The versions, once the installer has had them from the browser; the
    // browser keeps one list at a time, so this page keeps its own copy.
    property var versions: []
    property bool versionsLoading: true
    // The results moved before the versions came (a new search).
    property bool versionsLost: false
    property int picked: -1
    // Whether this page still exists when the versions come back.
    property var token: ({ alive: true })

    readonly property string key: page.installer ? page.installer.projectKey(page.project.title, page.project.author) : ""
    readonly property var download: page.installer ? page.installer.downloadFor(page.key) : null
    readonly property bool checking: !!page.installer && page.installer.isChecking(page.key)
    readonly property bool downloading: !!page.download && page.download.running
    readonly property bool busy: page.checking || page.downloading
    readonly property string blockedReason: page.installer ? page.installer.blockedReason : ""
    readonly property string projectUrl: page.browser && page.browser.provider === "modrinth" && (page.project.projectId || "").length > 0
                                         ? "https://modrinth.com/project/" + page.project.projectId : ""

    function install() {
        if (!page.installer || page.picked < 0 || page.busy)
            return
        page.installer.install(page.row, page.project, page.versions[page.picked].id)
    }
    // For review snapshots (DevRoute.qml's "version=<n>"): picks version
    // @p index once the versions are in.
    function pickVersion(index) {
        if (page.versionsLoading)
            return false
        if (index >= 0 && index < page.versions.length)
            page.picked = index
        return true
    }
    // For review snapshots (DevRoute.qml's "install").
    function installWhenReady() {
        if (page.versionsLoading)
            return false
        page.install()
        return true
    }

    title: page.project.title || ""

    Component.onCompleted: {
        if (!page.installer) {
            page.versionsLoading = false
            return
        }
        var token = page.token
        page.installer.requestVersions(page.row, function (versions) {
            if (!token.alive)
                return
            page.versionsLost = !versions
            page.versions = versions || []
            page.versionsLoading = false
            var best = page.installer.bestVersion(page.versions)
            page.picked = best ? page.versions.indexOf(best) : -1
        })
    }
    Component.onDestruction: page.token.alive = false

    actions: [
        Kirigami.Action {
            text: page.checking ? i18nc("@action:button", "Checking…")
                : page.downloading ? i18nc("@action:button", "Installing…")
                : page.project.installed ? i18nc("@action:button", "Install This Version")
                : i18nc("@action:button", "Install")
            icon.name: "download"
            enabled: page.picked >= 0 && !page.busy && page.blockedReason.length === 0 && !page.versionsLost
            onTriggered: page.install()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Open in Browser")
            icon.name: "internet-services"
            visible: page.projectUrl.length > 0
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: Qt.openUrlExternally(page.projectUrl)
        }
    ]

    footer: ProgressFooter {
        active: page.busy
        status: page.checking ? i18nc("@info:status", "Checking what it needs…")
              : page.download && page.download.status.length > 0 ? page.download.status
              : i18nc("@info:status", "Installing…")
        progress: page.downloading ? page.download.progress : -1
    }

    ListView {
        id: versionList
        model: page.versions
        currentIndex: page.picked

        header: ColumnLayout {
            width: versionList.width
            spacing: Kirigami.Units.largeSpacing

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                spacing: Kirigami.Units.largeSpacing * 2
                Kirigami.Icon {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                    Layout.preferredHeight: Kirigami.Units.iconSizes.huge
                    source: page.project.logoUrl || ""
                    fallback: "package-x-generic"
                    placeholder: "package-x-generic"
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: Kirigami.Units.smallSpacing
                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 1
                        text: page.project.title || ""
                        wrapMode: Text.Wrap
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: (page.project.author || "").length > 0
                        text: i18nc("@info %1 is a project's author", "by %1", page.project.author || "")
                        elide: Text.ElideRight
                        opacity: 0.7
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        visible: text.length > 0
                        text: page.project.description || ""
                        wrapMode: Text.Wrap
                    }
                }
            }

            Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                visible: page.blockedReason.length > 0
                type: Kirigami.MessageType.Information
                text: page.blockedReason
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                visible: !!page.download && !page.downloading && (page.download.succeeded || page.download.failed)
                type: page.download && page.download.failed ? Kirigami.MessageType.Error : Kirigami.MessageType.Positive
                text: !page.download ? ""
                    : page.download.failed ? (page.download.error.length > 0 ? i18n("The install failed: %1", page.download.error)
                                                                            : i18n("The install failed."))
                    : i18n("Installed. It is on the instance's Content tab.")
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                visible: page.versionsLost
                type: Kirigami.MessageType.Warning
                text: i18n("The search changed before this project's versions arrived. Go back and open it again.")
            }

            Kirigami.ListSectionHeader {
                Layout.fillWidth: true
                visible: page.versions.length > 0
                text: i18ncp("@title:group", "%1 Version", "%1 Versions", page.versions.length)
            }
        }

        delegate: KD.RadioSubtitleDelegate {
            id: entry
            required property var modelData
            required property int index

            readonly property string games: (entry.modelData.gameVersions || []).slice(0, 3).join(", ")
            readonly property string loaders: (entry.modelData.loaders || []).map(l => l.charAt(0).toUpperCase() + l.slice(1)).join(", ")
            readonly property string fits: entry.loaders.length > 0 && entry.games.length > 0
                                           ? i18nc("@info %1 mod loaders, %2 Minecraft versions", "%1 for %2", entry.loaders, entry.games)
                                           : entry.loaders + entry.games
            readonly property var published: Format.parseDate(entry.modelData.datePublished)

            width: ListView.view.width
            text: entry.modelData.versionNumber || entry.modelData.name || ""
            subtitle: {
                var parts = [entry.modelData.isCompatible ? entry.fits
                             : entry.fits.length > 0 ? i18nc("@info %1 what it runs on", "Not for this instance (%1)", entry.fits)
                             : i18n("Not for this instance")]
                if (entry.published)
                    parts.push(KCoreAddons.Format.formatRelativeDate(entry.published, Locale.ShortFormat))
                return parts.filter(p => p.length > 0).join(" · ")
            }
            opacity: entry.modelData.isCompatible ? 1 : 0.6
            checked: page.picked === entry.index
            enabled: !page.busy
            onToggled: {
                page.picked = entry.index
                checked = Qt.binding(() => page.picked === entry.index)
            }
        }

        Kirigami.LoadingPlaceholder {
            anchors.centerIn: parent
            visible: page.versionsLoading
            text: i18nc("@info:placeholder", "Loading versions…")
        }
        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: !page.versionsLoading && !page.versionsLost && page.versions.length === 0
            icon.name: "edit-find"
            text: i18n("No versions found")
            explanation: i18n("Nothing of this project can be installed on this instance.")
        }
    }
}
