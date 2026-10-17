// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.coreaddons as KCoreAddons
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

import "brands.js" as Brands
import "format.js" as Format

/*
 * One project from Add Content, pushed over it, laid out like a modpack's
 * page (ProjectHeader): its logo, name and author with Install under them
 * and, beside them, where it is from and what the picked version runs on;
 * what it is in a line and its own description, folded to a few lines;
 * then every version, newest first, the newest that runs on this instance
 * picked -- versions for another Minecraft version or loader are greyed and
 * say so. Install installs the picked one: what it needs is worked
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
    readonly property string site: page.browser ? page.browser.provider : ""
    readonly property string siteName: page.owner && page.owner.siteName ? page.owner.siteName : ""
    readonly property var pickedVersion: page.picked >= 0 && page.picked < page.versions.length ? page.versions[page.picked] : null
    // The project's own page: what the provider gives, else Modrinth's
    // address for its id.
    readonly property string projectUrl: {
        var website = page.browser && page.row >= 0 && typeof page.browser.projectWebsite === "function"
                      ? page.browser.projectWebsite(page.row) : ""
        if (website.length > 0)
            return website
        return page.site === "modrinth" && (page.project.projectId || "").length > 0
               ? "https://modrinth.com/project/" + page.project.projectId : ""
    }
    // Its long description, as HTML, once loaded with its versions; the
    // browser keeps it per row, this page its own copy.
    property string body: ""
    property bool bodyUnfolded: false
    function readBody() {
        if (!page.browser || page.versionsLost || page.row < 0 || typeof page.browser.projectBody !== "function")
            return
        var html = page.browser.projectBody(page.row)
        if (html.length > 0)
            page.body = html
    }
    Connections {
        target: page.browser
        ignoreUnknownSignals: true
        function onBodyLoaded(row) {
            if (row === page.row)
                page.readBody()
        }
    }

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
            page.readBody()
        })
    }
    Component.onDestruction: page.token.alive = false

    // Install is in the header, as on a modpack's page.
    actions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Open in Browser")
            icon.name: "internet-services"
            visible: page.projectUrl.length > 0
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
        // Picking a version must not scroll the header off the page.
        highlightFollowsCurrentItem: false
        // The header grows as the description and versions come in, and a
        // ListView keeps its rows where they were then, which pushes the
        // top of the header out of view: until the user scrolls, stay at
        // the top.
        property bool userScrolled: false
        onMovementStarted: versionList.userScrolled = true

        header: ColumnLayout {
            width: versionList.width
            onHeightChanged: {
                if (!versionList.userScrolled)
                    Qt.callLater(versionList.positionViewAtBeginning)
            }
            spacing: Kirigami.Units.largeSpacing

            // Who made it and where it is from, Install under the name,
            // and beside it what the picked version is and runs on.
            ProjectHeader {
                Layout.fillWidth: true
                iconSource: page.project.logoUrl || ""
                fallbackSource: "image://instanceicon/gear"
                title: page.project.title || ""
                author: page.project.author || ""
                logos: page.app.shell ? page.app.shell.brandLogos : null
                details: {
                    var list = [{ label: i18nc("@label where a project is from", "Source:"),
                                  text: page.siteName, brand: page.site }]
                    if (page.pickedVersion) {
                        list.push({ label: i18nc("@label:listbox", "Version:"),
                                    text: page.pickedVersion.versionNumber || page.pickedVersion.name || "" })
                        var loaders = page.pickedVersion.loaders || []
                        var games = (page.pickedVersion.gameVersions || []).slice(0, 3).join(", ")
                        list.push({ label: i18nc("@label what a version runs on", "Runs on:"),
                                    text: [loaders.map(l => l.charAt(0).toUpperCase() + l.slice(1)).join(", "), games]
                                          .filter(t => t.length > 0).join(" "),
                                    brand: loaders.length > 0 ? Brands.loaderId(loaders[0]) : "" })
                    }
                    if (page.project.installed)
                        list.push({ label: i18nc("@label", "Installed:"),
                                    text: i18nc("@info a project is installed on the instance", "On this instance") })
                    return list
                }

                controls: QQC2.Button {
                    text: page.checking ? i18nc("@action:button", "Checking…")
                        : page.downloading ? i18nc("@action:button", "Installing…")
                        : page.project.installed ? i18nc("@action:button", "Install This Version")
                        : i18nc("@action:button", "Install")
                    icon.name: "download-symbolic"
                    enabled: page.picked >= 0 && !page.busy && page.blockedReason.length === 0 && !page.versionsLost
                    onClicked: page.install()
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

            // What it is in a line, then its own description, folded to a
            // few lines until asked for.
            Kirigami.SelectableLabel {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.gridUnit
                Layout.rightMargin: Kirigami.Units.gridUnit
                visible: (page.project.description || "").length > 0
                text: page.project.description || ""
                wrapMode: Text.Wrap
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.35
                font.weight: Font.DemiBold
                Accessible.role: Accessible.Heading
            }
            Item {
                id: bodyFrame
                readonly property real foldedHeight: Kirigami.Units.gridUnit * 10
                readonly property bool foldable: description.implicitHeight > bodyFrame.foldedHeight + Kirigami.Units.gridUnit * 2
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.gridUnit
                Layout.rightMargin: Kirigami.Units.gridUnit
                Layout.maximumWidth: Kirigami.Units.gridUnit * 40
                implicitHeight: bodyFrame.foldable && !page.bodyUnfolded ? bodyFrame.foldedHeight : description.implicitHeight
                visible: page.body.length > 0
                clip: true
                // Not a Label: see RemoteRichText.
                RemoteRichText {
                    id: description
                    width: parent.width
                    source: page.body
                    format: "html"
                    article: true
                }
            }
            QQC2.Button {
                Layout.leftMargin: Kirigami.Units.gridUnit
                visible: page.body.length > 0 && bodyFrame.foldable
                flat: true
                text: page.bodyUnfolded ? i18nc("@action:button", "Show Less") : i18nc("@action:button", "Show More")
                icon.name: page.bodyUnfolded ? "go-up-symbolic" : "go-down-symbolic"
                onClicked: page.bodyUnfolded = !page.bodyUnfolded
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
