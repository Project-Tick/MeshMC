// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * Modpacks to install as new instances, from Modrinth, CurseForge, FTB,
 * ATLauncher or Technic: the provider in the tab bar under the title, the
 * search beside the title, sort and filters in the toolbar, and the results
 * as a list that loads more as it is scrolled. A pack opens on its own page
 * (ModpackPage), pushed over this one.
 *
 * Each provider has its own model on the shell, all with the same
 * properties, invokables and roles -- query, loader and sort in, search()
 * and fetchMore() to run, rows out, and a lazily loaded `detail` -- so the
 * page works on whichever the tab bar picked. Each keeps its own results.
 * Filters a provider cannot answer (a loader on ATLauncher or Technic, a
 * sort on Technic) are left out rather than shown doing nothing.
 *
 * Nothing is asked of the network until the page is first shown.
 */
Kirigami.ScrollablePage {
    id: page

    required property var app
    readonly property var shell: page.app.shell

    // Every provider the shell has a model for, in the tab bar's order.
    readonly property var providers: {
        var shell = page.shell
        var all = [
            { id: "modrinth", name: "Modrinth", model: shell ? shell.modpackModel : null },
            { id: "curseforge", name: "CurseForge", model: shell ? shell.curseForgeModpackModel : null },
            { id: "ftb", name: "FTB", model: shell ? shell.ftbModpackModel : null },
            { id: "atlauncher", name: "ATLauncher", model: shell ? shell.atlModpackModel : null },
            { id: "technic", name: "Technic", model: shell ? shell.technicModpackModel : null }
        ]
        return all.filter(p => !!p.model)
    }
    readonly property var current: page.providers.find(p => p.id === page.app.discoverProvider)
                                   || page.providers[0] || null
    readonly property var model: page.current ? page.current.model : null
    readonly property string providerName: page.current ? page.current.name : ""
    readonly property bool keyMissing: !!page.model && page.model.keyMissing === true
    readonly property bool searching: !!page.model && page.model.searching
    readonly property string error: page.model && page.model.error ? page.model.error : ""
    readonly property int count: page.model ? page.model.count : 0
    readonly property bool searched: !!page.current && page.app.discoverSearched.indexOf(page.current.id) >= 0
    // ATLauncher and Technic say nothing about loaders.
    readonly property bool loaderFilterAvailable: !!page.current
                                                  && page.current.id !== "atlauncher" && page.current.id !== "technic"
    readonly property var sortOptions: page.model && page.model.sortOptions ? page.model.sortOptions : []
    // What the search field holds; the model's query follows it once the
    // typing pauses.
    property string searchText: ""

    // Modrinth installs through the shell; the other models install
    // themselves. Either way the TaskWatcher belongs to C++.
    function installerFor(provider) {
        var shell = page.shell
        if (provider.id === "modrinth")
            return (projectId, versionId, name, group) => shell.installModpack(projectId, versionId, name, group)
        return (projectId, versionId, name, group) => provider.model.install(projectId, versionId, name, group)
    }

    function runSearch() {
        if (!page.model || page.keyMissing)
            return
        if (!page.searched)
            page.app.discoverSearched = page.app.discoverSearched.concat([page.current.id])
        page.model.search()
    }
    function setSearch(text) {
        page.searchText = text
        if (page.model && page.model.query !== text) {
            page.model.query = text
            page.runSearch()
        }
    }
    // Switches provider, carrying the search words and loader across, and
    // searches only if the other provider's results do not already answer
    // them.
    function setProvider(id) {
        if (!page.providers.some(p => p.id === id) || page.current.id === id)
            return
        var query = page.searchText
        var loader = page.model ? page.model.loader : ""
        page.app.discoverProvider = id
        page.searchCurrent(query, loader)
    }
    function searchCurrent(query, loader) {
        if (!page.model)
            return
        var stale = page.model.query !== query || (page.loaderFilterAvailable && page.model.loader !== loader)
        page.model.query = query
        if (page.loaderFilterAvailable)
            page.model.loader = loader
        if (!page.searched || stale)
            page.runSearch()
    }

    // Result @p index's page (DevRoute.qml's "detail=<n>"): false while
    // there is no such result yet.
    function openResult(index) {
        var item = results.itemAtIndex(index)
        if (!item || page.searching)
            return false
        item.clicked()
        return true
    }
    function openFilters() {
        if (!page.loaderFilterAvailable || !page.model)
            return false
        filterDialog.open()
        return true
    }
    function open(row) {
        page.app.openModpack({
            provider: page.current.id,
            providerName: page.providerName,
            model: page.model,
            installer: page.installerFor(page.current),
            bodyFormat: page.model.bodyFormat || "markdown"
        }, {
            projectId: row.projectId,
            slug: row.slug || "",
            title: row.title || "",
            author: row.author || "",
            description: row.description || "",
            logoUrl: row.logoUrl || "",
            downloads: row.downloads || 0,
            follows: row.follows || 0,
            updated: row.updated || "",
            categories: row.categories || [],
            galleryUrl: row.galleryUrl || ""
        })
    }

    Component.onCompleted: {
        if (page.model)
            page.searchText = page.model.query
        if (!page.searched)
            page.runSearch()
    }
    // Another provider picked from outside (a link, the dev route): its
    // first search, unless setProvider() has just made it.
    function searchIfNew() {
        if (!page.searched)
            page.runSearch()
    }
    onCurrentChanged: Qt.callLater(page.searchIfNew)

    title: i18nc("@title", "Discover")
    titleDelegate: SearchHeading {
        title: page.title
        text: page.searchText
        fieldEnabled: !!page.model && !page.keyMissing
        placeholderText: i18nc("@info:placeholder %1 is a site such as Modrinth", "Search %1…", page.providerName)
        current: page.isCurrentPage
        onEdited: (text) => page.searchText = text
        onSearchRequested: (text) => page.setSearch(text)
    }

    // The providers as tabs, once there is more than one. A Loader, since
    // the desktop style's TabBar cannot be empty.
    header: Loader {
        active: page.providers.length > 1
        sourceComponent: QQC2.TabBar {
            id: providerBar
            currentIndex: Math.max(0, page.providers.indexOf(page.current))
            Repeater {
                model: page.providers
                QQC2.TabButton {
                    required property var modelData
                    text: modelData.name
                    width: implicitWidth
                    onClicked: page.setProvider(modelData.id)
                }
            }
            Connections {
                target: page
                function onCurrentChanged() {
                    providerBar.currentIndex = Math.max(0, page.providers.indexOf(page.current))
                }
            }
        }
    }

    // The provider's own sorts; one sort is no choice (Technic).
    SortAction {
        id: sortAction
        options: page.sortOptions
        current: page.model ? page.model.sort : undefined
        onPicked: (id) => {
            page.model.sort = id
            page.runSearch()
        }
    }
    Kirigami.Action {
        id: filtersAction
        readonly property bool narrowed: !!page.model && page.loaderFilterAvailable && page.model.loader.length > 0
        text: narrowed ? i18nc("@action:button %1 is a number of filters", "Filters (%1)", 1)
                       : i18nc("@action:button", "Filters")
        icon.name: "view-filter"
        visible: page.loaderFilterAvailable && !page.keyMissing
        onTriggered: page.openFilters()
    }
    Kirigami.Action {
        id: importAction
        text: i18nc("@action:button", "Import…")
        icon.name: "document-import"
        tooltip: i18n("Import a pack exported from CurseForge, FTB, ATLauncher, Technic or another launcher")
        onTriggered: page.app.openImport()
    }
    actions: [sortAction, filtersAction, importAction]

    ListView {
        id: results
        // Rows are opened by a click; none is picked to begin with.
        currentIndex: -1
        model: page.keyMissing ? null : page.model
        delegate: ModpackRow {
            width: ListView.view.width
            onClicked: page.open(model)
        }

        // QML views never ask for more on their own: ask as the end comes
        // into view, or when the first page does not even fill it.
        function fetchMoreIfNeeded() {
            if (results.atYEnd && page.model && page.model.canFetchMore && !page.searching)
                page.model.fetchMore()
        }
        onAtYEndChanged: results.fetchMoreIfNeeded()
        onCountChanged: Qt.callLater(results.fetchMoreIfNeeded)

        // A further page on its way.
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
            text: i18nc("@info:placeholder %1 is a site such as Modrinth", "Searching %1…", page.providerName)
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: !page.searching && page.count === 0 && (page.searched || page.keyMissing || !page.model)
            icon.name: !page.model ? "system-search"
                     : page.keyMissing || page.error.length > 0 ? "network-disconnect"
                     : "edit-find"
            text: !page.model ? i18n("Nothing to browse")
                : page.keyMissing ? i18n("%1 isn't available", page.providerName)
                : page.error.length > 0 ? i18n("Couldn't reach %1", page.providerName)
                : i18n("No modpacks found")
            explanation: !page.model ? i18n("This build of MeshMC has no modpack catalogues.")
                       : page.keyMissing ? i18n("This build has no %1 API key, so its modpacks can't be browsed here.", page.providerName)
                       : page.error.length > 0 ? page.error
                       : i18n("Try fewer words, or another mod loader.")
            helpfulAction: page.keyMissing && page.providers.length > 1 ? browseModrinthAction
                         : page.error.length > 0 ? retryAction : null
        }
    }

    Kirigami.Action {
        id: retryAction
        text: i18nc("@action:button", "Try Again")
        icon.name: "view-refresh"
        onTriggered: page.runSearch()
    }
    Kirigami.Action {
        id: browseModrinthAction
        text: i18nc("@action:button", "Browse Modrinth")
        icon.name: "go-next"
        onTriggered: page.setProvider("modrinth")
    }

    ModpackFilterDialog {
        id: filterDialog
        model: page.model
        onChanged: page.runSearch()
    }
}
