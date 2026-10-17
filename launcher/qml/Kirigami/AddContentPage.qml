// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import "brands.js" as Brands

/*
 * Mods, resource packs, shaders and data packs for one instance, from
 * Modrinth or CurseForge, pushed over the instance's page by its Content
 * tab: the kind in the tab bar under the title, the search beside the
 * title, the site, sort and filters in the toolbar. Results start out
 * matching the instance's Minecraft version and loader, and say what is
 * installed already.
 *
 * Install on a row installs the newest version that runs here, with what
 * it needs; clicking the row opens the project (ContentProjectPage) to pick
 * another version. Installs go on side by side, each on its row, with the
 * total in the footer; they belong to the instance page's ContentInstaller,
 * so they carry on after going back.
 *
 * `details` is the instance's InstanceDetails; its contentBrowser
 * (ContentBrowser) does the searching, made only once this page opens.
 */
Kirigami.ScrollablePage {
    id: page

    required property var app
    property var details: null
    // The instance page's ContentInstaller.
    property var installer: null
    // The kind to open on (the one the Content tab showed), if browsable.
    property string initialKind: ""

    readonly property var browser: page.installer ? page.installer.browser : null
    readonly property bool searching: !!page.browser && page.browser.searching
    readonly property int count: page.browser ? page.browser.count : 0
    readonly property string error: page.browser ? page.browser.error : ""
    readonly property bool keyMissing: !!page.browser && page.browser.provider === "curseforge"
                                       && page.browser.curseForgeKeyMissing
    property bool searched: false
    property string searchText: ""

    readonly property var kinds: [
        { id: "mods", text: i18nc("@title:tab", "Mods") },
        { id: "resourcepacks", text: i18nc("@title:tab", "Resource Packs") },
        { id: "shaderpacks", text: i18nc("@title:tab", "Shaders") },
        { id: "datapacks", text: i18nc("@title:tab", "Data Packs") }
    ]
    readonly property var sites: [
        { id: "modrinth", text: "Modrinth" },
        { id: "curseforge", text: "CurseForge" }
    ]
    readonly property string kind: page.browser ? page.browser.contentType : "mods"
    readonly property string site: page.browser ? page.browser.provider : "modrinth"
    readonly property string siteName: (page.sites.find(s => s.id === page.site) || page.sites[0]).text
    readonly property var sortOptions: page.browser && page.browser.sortOptions ? page.browser.sortOptions : []

    function runSearch() {
        if (!page.browser || page.keyMissing)
            return
        page.searched = true
        page.installer.resultsMoved()
        page.browser.search()
    }
    function setSearch(text) {
        page.searchText = text
        if (page.browser) {
            page.browser.query = text
            page.runSearch()
        }
    }
    // Opens on @p kind when it is one that can be browsed, and searches it.
    function showKind(kind) {
        if (!page.browser || !page.kinds.some(k => k.id === kind) || page.browser.contentType === kind)
            return
        page.browser.contentType = kind
        page.runSearch()
    }
    function setProvider(id) {
        if (!page.browser || page.browser.provider === id || !page.sites.some(s => s.id === id))
            return
        page.browser.provider = id
        page.runSearch()
    }
    function openFilters() {
        if (!page.browser)
            return false
        filterDialog.open()
        return true
    }
    // Result @p index's project (DevRoute.qml's "detail=<n>"): false while
    // there is no such result yet.
    function openResult(index) {
        var item = results.itemAtIndex(index)
        if (!item || page.searching)
            return false
        item.clicked()
        return true
    }

    Component.onCompleted: {
        if (page.installer)
            page.installer.active = true
        if (page.browser) {
            if (page.kinds.some(k => k.id === page.initialKind))
                page.browser.contentType = page.initialKind
            page.searchText = page.browser.query
            // Back on the results of last time, or a first search.
            if (page.count === 0 && !page.searching)
                page.runSearch()
            else
                page.searched = true
        }
    }

    Component.onDestruction: {
        if (page.app.instancePage && page.browser)
            page.app.instancePage.showContentKind(page.browser.contentType)
    }

    // A filter changed: the browser searches again by itself, and the rows
    // move.
    Connections {
        target: page.browser
        function onFiltersChanged() {
            page.searched = true
            page.installer.resultsMoved()
        }
    }

    title: i18nc("@title", "Add Content")
    titleDelegate: SearchHeading {
        title: page.title
        text: page.searchText
        fieldEnabled: !!page.browser && !page.keyMissing
        placeholderText: i18nc("@info:placeholder %1 is a site such as Modrinth", "Search %1…", page.siteName)
        current: page.isCurrentPage
        onEdited: (text) => page.searchText = text
        onSearchRequested: (text) => page.setSearch(text)
    }

    header: QQC2.TabBar {
        id: kindBar
        currentIndex: Math.max(0, page.kinds.findIndex(k => k.id === page.kind))
        Repeater {
            model: page.kinds
            QQC2.TabButton {
                required property var modelData
                text: modelData.text
                width: implicitWidth
                onClicked: page.showKind(modelData.id)
            }
        }
        Connections {
            target: page
            function onKindChanged() {
                kindBar.currentIndex = Math.max(0, page.kinds.findIndex(k => k.id === page.kind))
            }
        }
    }

    readonly property var logos: page.app.shell ? page.app.shell.brandLogos : null
    function siteLogo(site) {
        return Brands.source(site, page.logos)
    }

    // Where to look: Modrinth or CurseForge, each by its logo (a plain
    // globe while one is still to be fetched).
    component SiteAction: Kirigami.Action {
        required property string site
        icon.source: page.siteLogo(site)
        icon.name: page.siteLogo(site).length > 0 ? "" : "globe-symbolic"
        checkable: true
        checked: page.site === site
        onTriggered: {
            checked = Qt.binding(() => page.site === site)
            page.setProvider(site)
        }
    }

    SortAction {
        id: sortAction
        options: page.sortOptions
        current: page.browser ? page.browser.sortIndex : undefined
        onPicked: (id) => {
            page.browser.sortIndex = id
            page.runSearch()
        }
    }
    Kirigami.Action {
        id: siteAction
        text: page.siteName
        icon.source: page.siteLogo(page.site)
        icon.name: page.siteLogo(page.site).length > 0 ? "" : "globe-symbolic"
        tooltip: i18nc("@info:tooltip", "Where to search")
        SiteAction {
            site: "modrinth"
            text: "Modrinth"
        }
        SiteAction {
            site: "curseforge"
            text: "CurseForge"
        }
    }
    Kirigami.Action {
        id: filtersAction
        readonly property int active: page.browser ? page.browser.activeFilterCount : 0
        text: active > 0 ? i18nc("@action:button %1 is a number of filters", "Filters (%1)", active)
                         : i18nc("@action:button", "Filters")
        icon.name: "view-filter"
        enabled: !!page.browser && !page.keyMissing
        onTriggered: page.openFilters()
    }
    actions: [siteAction, sortAction, filtersAction]

    // Installs in flight, together, while browsing on.
    footer: ProgressFooter {
        active: !!page.installer && page.installer.activeCount > 0
        status: page.installer ? i18ncp("@info:status", "Installing %1 project…", "Installing %1 projects…",
                                        page.installer.activeCount) : ""
        progress: page.installer ? page.installer.activeProgress : -1
    }

    ListView {
        id: results
        // Rows are opened by a click; none is picked to begin with.
        currentIndex: -1
        model: page.browser && !page.keyMissing ? page.browser.results : null
        // The notices go above the rows, inside the scrolling list.
        header: ColumnLayout {
            width: results.width
            spacing: 0
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.smallSpacing
                visible: !!page.installer && page.installer.blockedReason.length > 0
                type: Kirigami.MessageType.Information
                text: page.installer ? page.installer.blockedReason : ""
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.smallSpacing
                visible: !!page.browser && page.kind === "mods" && !page.browser.hasModLoader
                type: Kirigami.MessageType.Warning
                text: i18n("This instance has no mod loader, so mods will not run. Install Fabric, Forge, NeoForge or Quilt for it first.")
            }
        }
        delegate: ContentResultRow {
            width: ListView.view.width
            installer: page.installer
            onClicked: page.app.openContentProject(page, index, project())
        }

        function fetchMoreIfNeeded() {
            if (results.atYEnd && page.browser && page.browser.canFetchMore && !page.searching)
                page.browser.fetchMore()
        }
        onAtYEndChanged: results.fetchMoreIfNeeded()
        onCountChanged: Qt.callLater(results.fetchMoreIfNeeded)

        footer: Item {
            width: results.width
            height: page.searching && page.count > 0 ? Kirigami.Units.gridUnit * 3 : 0
            QQC2.BusyIndicator {
                anchors.centerIn: parent
                running: page.searching && page.count > 0
                visible: running
            }
        }

        Kirigami.LoadingPlaceholder {
            anchors.centerIn: parent
            visible: page.searching && page.count === 0
            text: i18nc("@info:placeholder %1 is a site such as Modrinth", "Searching %1…", page.siteName)
        }
        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: !page.searching && page.count === 0 && (page.searched || page.keyMissing || !page.browser)
            icon.name: !page.browser ? "package-x-generic"
                     : page.keyMissing || page.error.length > 0 ? "network-disconnect" : "edit-find"
            text: !page.browser ? i18n("Nothing to add here")
                : page.keyMissing ? i18n("CurseForge isn't available")
                : page.error.length > 0 ? i18n("The search failed")
                : i18n("Nothing found")
            explanation: !page.browser ? i18n("This instance can't take content from Modrinth or CurseForge.")
                       : page.keyMissing ? i18n("This build has no CurseForge API key, so its content can't be browsed here.")
                       : page.error.length > 0 ? page.error
                       : i18n("Try other words, fewer filters, or the other site.")
            helpfulAction: page.keyMissing ? useModrinthAction : page.error.length > 0 ? retryAction : null
        }
    }

    Kirigami.Action {
        id: retryAction
        text: i18nc("@action:button", "Try Again")
        icon.name: "view-refresh"
        onTriggered: page.runSearch()
    }
    Kirigami.Action {
        id: useModrinthAction
        text: i18nc("@action:button", "Search Modrinth")
        icon.name: "go-next"
        onTriggered: page.setProvider("modrinth")
    }

    ContentFilterDialog {
        id: filterDialog
        browser: page.browser
    }
}
