// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "combos.js" as Combos

/*
 * One modpack, pushed over Discover, laid out as KDE's software centre lays
 * out an application: a header band with its logo, name, author and
 * figures, Install with the version to install and the instance name under
 * them, and where it is from and what it runs on beside them; then its
 * pictures across the page, what it is in a line, its own description and
 * where else it lives. Install makes a new instance from it; the footer
 * shows how that is going, and once it is done the page offers the new
 * instance.
 *
 * `pack` is the result row that was opened; the provider's model loads the
 * rest -- versions, gallery, body, links -- into its one `detail` (a
 * ModrinthModpackDetail, whatever the provider), which this page asks for
 * when it opens. Every provider writes its body as one HTML article
 * (RichBody), shown with RemoteRichText.
 *
 * The install itself belongs to C++ (the TaskWatcher) and to the window,
 * which keeps it by pack (Main.qml's modpackInstalls): it goes on when the
 * page is closed, and the page finds it again when the pack is reopened.
 * An installed pack the launcher already has an instance of is asked about
 * by the import itself ("update it instead?", through UiRequests).
 */
Kirigami.ScrollablePage {
    id: page

    required property var app
    // DiscoverPage's {provider, providerName, model, installer, bodyFormat}.
    property var source: ({})
    // The result row: projectId, slug, title, author, description, logoUrl,
    // downloads, follows, updated, categories, galleryUrl.
    property var pack: ({})

    readonly property var model: page.source.model || null
    readonly property var detail: page.model ? page.model.detail : null
    // The detail is shared by every pack of the provider: only read it while
    // it is this pack's.
    readonly property bool detailIsOurs: !!page.detail && String(page.detail.projectId) === String(page.pack.projectId)
    readonly property bool loading: !page.detailIsOurs || page.detail.loading
    readonly property var versions: page.detailIsOurs ? page.detail.versions : []
    readonly property var gallery: page.detailIsOurs && page.detail.gallery ? page.detail.gallery : []
    readonly property string bodyHtml: page.detailIsOurs ? (page.detail.bodyHtml || "") : ""
    readonly property string detailError: page.detailIsOurs ? (page.detail.error || "") : ""

    readonly property string installKey: (page.source.provider || "") + "/" + (page.pack.projectId || "")
    readonly property var watcher: {
        page.app.modpackInstallsRevision
        return page.app.modpackInstalls[page.installKey] || null
    }
    readonly property bool installing: !!page.watcher && page.watcher.running
    readonly property bool installed: !!page.watcher && page.watcher.succeeded
    readonly property bool installFailed: !!page.watcher && page.watcher.failed

    // Loader ids as the projects spell their own names.
    readonly property var loaderNames: ({ neoforge: "NeoForge", forge: "Forge", fabric: "Fabric",
                                          quilt: "Quilt", liteloader: "LiteLoader" })
    function versionLabel(v) {
        var parts = []
        if (v.gameVersions && v.gameVersions.length > 0)
            parts.push(v.gameVersions[0])
        if (v.loaders && v.loaders.length > 0)
            parts.push(v.loaders.map(l => page.loaderNames[l] || (l.charAt(0).toUpperCase() + l.slice(1))).join(", "))
        // Many packs number their versions with the pack's own name in front
        // ("All the Mods 10-8.3"); repeating it on every entry only pushes
        // the part that differs to the end. Show "8.3" instead.
        var label = String(v.versionNumber || v.name || "")
        var title = String(page.pack.title || "")
        if (title.length > 0 && label.toLowerCase().startsWith(title.toLowerCase())) {
            var rest = label.slice(title.length).replace(/^[\s\-_:v]+/i, "")
            if (rest.length > 0)
                label = rest
        }
        return parts.length > 0 ? i18nc("@item:inlistbox %1 version, %2 what it runs on", "%1 (%2)", label, parts.join(", "))
                                : label
    }

    function install() {
        if (!page.source.installer || versionBox.currentIndex < 0 || page.installing)
            return
        var version = page.versions[versionBox.currentIndex]
        var name = nameField.text.trim().length > 0 ? nameField.text.trim() : page.pack.title
        var watcher = page.source.installer(page.pack.projectId, version.id, name, "")
        if (!watcher) {
            page.app.notify(i18n("This version can't be installed."))
            return
        }
        page.app.trackModpackInstall(page.installKey, watcher, name)
    }
    // For review snapshots (DevRoute.qml's "install"): once the versions
    // are in.
    function installWhenReady() {
        if (page.versions.length === 0 || versionBox.currentIndex < 0)
            return false
        if (!page.installing)
            page.install()
        return true
    }
    function openGallery(index) {
        if (index < 0 || index >= page.gallery.length)
            return false
        galleryViewer.show(index)
        return true
    }
    // For review snapshots (DevRoute.qml's "scroll=<where>").
    function scrollTo(where) {
        var flick = page.flickable
        if (!flick)
            return false
        var y
        if (where === "description") {
            if (page.bodyHtml.length === 0 || description.height <= 0)
                return false
            y = description.mapToItem(flick.contentItem, 0, 0).y - Kirigami.Units.largeSpacing
        } else if (where === "bottom") {
            if (page.loading)
                return false
            y = flick.contentHeight
        } else {
            y = parseInt(where)
        }
        flick.contentY = Math.max(0, Math.min(y, flick.contentHeight - flick.height))
        return true
    }

    title: page.pack.title || ""

    Component.onCompleted: {
        nameField.text = page.pack.title || ""
        if (page.model && !page.detailIsOurs)
            page.model.loadDetail(page.pack.projectId)
    }

    // Install is in the header, beside its choices, as in KDE's software
    // centre; the toolbar keeps what is about the page itself.
    actions: [
        Kirigami.Action {
            readonly property var link: linksSection.pageLink
            text: i18nc("@action:button", "Open in Browser")
            icon.name: "internet-services"
            visible: !!link
            onTriggered: Qt.openUrlExternally(link.url)
        }
    ]

    // The install's progress while it runs, as the instance page shows a
    // launch's.
    footer: ProgressFooter {
        active: page.installing
        status: page.watcher && page.watcher.status.length > 0 ? page.watcher.status
                                                              : i18nc("@info:status", "Installing…")
        progress: page.watcher ? page.watcher.progress : -1
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing * 2

        // Who made it, how popular it is, where it is from and what it runs
        // on, with Install and its choices under the name.
        ModpackHeader {
            Layout.fillWidth: true
            Layout.topMargin: -page.topPadding
            Layout.leftMargin: -page.leftPadding
            Layout.rightMargin: -page.rightPadding
            horizontalPadding: page.leftPadding
            pack: page.pack
            providerId: page.source.provider || ""
            providerName: page.source.providerName || ""
            version: versionBox.currentIndex >= 0 && versionBox.currentIndex < page.versions.length
                     ? page.versions[versionBox.currentIndex] : null
            loaderNames: page.loaderNames
            logos: page.app.shell ? page.app.shell.brandLogos : null

            controls: [
                RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    QQC2.Button {
                        visible: !page.installed
                        text: page.installing ? i18nc("@action:button", "Installing…") : i18nc("@action:button", "Install")
                        icon.name: "download-symbolic"
                        enabled: !page.installing && page.versions.length > 0 && versionBox.currentIndex >= 0
                        onClicked: page.install()
                    }
                    QQC2.Button {
                        visible: page.installed && (page.watcher.instanceId || "").length > 0
                        text: i18nc("@action:button", "Open Instance")
                        icon.name: "go-next"
                        onClicked: page.app.openInstance(page.watcher.instanceId)
                    }
                },
                // What Install installs: the featured version (or the newest)
                // and the pack's own name, until changed.
                Kirigami.FormLayout {
                    Layout.fillWidth: true
                    wideMode: true

                    QQC2.ComboBox {
                        id: versionBox
                        // A long list scrolls under the box (combos.js).
                        Component.onCompleted: Combos.capPopup(versionBox, Kirigami.Units.gridUnit)
                        Kirigami.FormData.label: i18nc("@label:listbox", "Version:")
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 18
                        enabled: page.versions.length > 0 && !page.installing
                        model: page.versions.map(v => page.versionLabel(v))
                        displayText: page.loading && page.versions.length === 0 ? i18nc("@item:inlistbox", "Loading…")
                                   : page.versions.length === 0 ? i18nc("@item:inlistbox", "No versions")
                                   : currentText
                        // Newest first from every provider, alphas included:
                        // start on the version the author features, if any.
                        onModelChanged: {
                            var featured = page.versions.findIndex(v => v.featured)
                            currentIndex = featured >= 0 ? featured : (page.versions.length > 0 ? 0 : -1)
                        }
                    }
                    QQC2.TextField {
                        id: nameField
                        Kirigami.FormData.label: i18nc("@label:textbox", "Instance name:")
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 18
                        enabled: !page.installing
                        onAccepted: page.install()
                    }
                }
            ]
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: page.installed
            type: Kirigami.MessageType.Positive
            text: i18n("“%1” is in your library.", page.watcher ? page.watcher.title : "")
            actions: Kirigami.Action {
                text: i18nc("@action:button", "Open Instance")
                icon.name: "go-next"
                visible: !!page.watcher && (page.watcher.instanceId || "").length > 0
                onTriggered: page.app.openInstance(page.watcher.instanceId)
            }
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: page.installFailed
            type: Kirigami.MessageType.Error
            text: page.watcher && page.watcher.error.length > 0
                  ? i18n("The install failed: %1", page.watcher.error) : i18n("The install failed.")
            actions: Kirigami.Action {
                text: i18nc("@action:button", "Try Again")
                icon.name: "view-refresh"
                enabled: page.versions.length > 0
                onTriggered: page.install()
            }
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: page.detailError.length > 0
            type: Kirigami.MessageType.Warning
            text: i18n("Some of this pack's details could not be loaded: %1", page.detailError)
            actions: Kirigami.Action {
                text: i18nc("@action:button", "Try Again")
                icon.name: "view-refresh"
                onTriggered: page.model.loadDetail(page.pack.projectId)
            }
        }

        // Its pictures, a row across the page; one opens full size.
        GalleryStrip {
            Layout.fillWidth: true
            Layout.leftMargin: -page.leftPadding
            Layout.rightMargin: -page.rightPadding
            visible: page.gallery.length > 0
            edgeMargin: page.leftPadding
            images: page.gallery
            onOpened: (index) => galleryViewer.show(index)
        }

        // What it is in a line, then its own description.
        Kirigami.SelectableLabel {
            Layout.fillWidth: true
            visible: (page.pack.description || "").length > 0
            text: page.pack.description || ""
            wrapMode: Text.Wrap
            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.35
            font.weight: Font.DemiBold
            Accessible.role: Accessible.Heading
        }
        Kirigami.LoadingPlaceholder {
            Layout.alignment: Qt.AlignHCenter
            visible: page.loading
        }
        // Not a Label: the body links images from anywhere, and a failed one
        // could crash Text (see RemoteRichText).
        RemoteRichText {
            id: description
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 40
            visible: page.bodyHtml.length > 0
            source: page.bodyHtml
            format: "html"
            article: true
        }

        ModpackLinks {
            id: linksSection
            Layout.fillWidth: true
            Layout.bottomMargin: Kirigami.Units.largeSpacing
            detailLinks: page.detailIsOurs && page.detail.links ? page.detail.links : []
            provider: page.source.provider || ""
            providerName: page.source.providerName || ""
            slug: page.pack.slug || ""
            logos: page.app.shell ? page.app.shell.brandLogos : null
        }
    }

    GalleryViewer {
        id: galleryViewer
        images: page.gallery
    }
}
