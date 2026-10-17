// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * The start page: every instance, in its group, as a grid of cards or a
 * list. The header has the search field; its toolbar creates and imports
 * instances, sorts them and switches between grid and list.
 *
 * The page never builds models itself: the search-filtered instance list
 * and one model per group are the shell's C++ proxies, and the sort order
 * is the launcher-wide InstSortMode setting they already follow.
 */
Kirigami.ScrollablePage {
    id: page

    required property var app
    readonly property var instanceModel: page.app.instanceModel
    readonly property var shell: page.app.shell
    readonly property var library: page.shell && page.shell.library ? page.shell.library : null
    readonly property var settings: page.shell && page.shell.settings ? page.shell.settings : null

    readonly property var groups: page.instanceModel && page.instanceModel.groups ? page.instanceModel.groups : []
    readonly property int instanceCount: page.instanceModel && page.instanceModel.count !== undefined
                                         ? page.instanceModel.count : 0
    readonly property string searchText: page.instanceModel && page.instanceModel.filterText !== undefined
                                         ? page.instanceModel.filterText : ""
    readonly property bool searching: page.searchText.length > 0
    // Folded groups, kept in the instance groups file; ignored while
    // searching, so a match is never hidden in a folded group.
    readonly property var collapsedGroups: page.library ? page.library.collapsedGroups : []

    function setSearch(text) {
        if (page.instanceModel && page.instanceModel.filterText !== text)
            page.instanceModel.filterText = text
    }

    // The context menu of instance @p id, for review snapshots (Main.qml's
    // "menu=<id>" route): opened under the page's top-left corner, since
    // there is no pointer to open it at.
    function openMenuFor(id) {
        var info = page.library ? page.library.describe(id) : ({})
        if (!info.name)
            return
        page.app.dialogs.openMenu({
            instanceId: id,
            name: info.name,
            iconKey: info.iconKey || "",
            group: info.group || "",
            running: page.shell.isInstanceRunning(id),
            canLaunch: true
        }, groupList, Qt.point(Kirigami.Units.gridUnit, Kirigami.Units.gridUnit))
    }

    // InstSortMode: "Name", "LastLaunch" or "TotalTimePlayed", as the
    // classic window's Settings writes it too.
    property string sortMode: "Name"
    function readSortMode() {
        page.sortMode = page.settings ? String(page.settings.value("InstSortMode") || "Name") : "Name"
    }
    function setSortMode(mode) {
        if (page.settings)
            page.settings.setValue("InstSortMode", mode)
        page.readSortMode()
    }
    Connections {
        target: page.settings
        function onValueChanged(id, value) {
            if (id === "InstSortMode")
                page.readSortMode()
        }
    }
    Component.onCompleted: page.readSortMode()

    component SortAction: Kirigami.Action {
        required property string mode
        checkable: true
        checked: page.sortMode === mode
        onTriggered: {
            // A checkable action unchecks itself on a second click.
            checked = Qt.binding(() => page.sortMode === mode)
            page.setSortMode(mode)
        }
    }
    component ViewAction: Kirigami.Action {
        required property string view
        checkable: true
        checked: page.app.libraryView === view
        displayHint: Kirigami.DisplayHint.IconOnly
        onTriggered: {
            checked = Qt.binding(() => page.app.libraryView === view)
            page.app.libraryView = view
        }
    }

    title: i18nc("@title", "Library")
    titleDelegate: RowLayout {
        spacing: Kirigami.Units.largeSpacing
        Kirigami.Heading {
            text: page.title
            elide: Text.ElideRight
            Layout.minimumWidth: 0
        }
        Kirigami.SearchField {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 14
            Layout.minimumWidth: Kirigami.Units.gridUnit * 6
            placeholderText: i18nc("@info:placeholder", "Search instances…")
            text: page.searchText
            onTextChanged: page.setSearch(text)
            // Only the page on screen answers Ctrl+F.
            focusSequences: page.isCurrentPage ? [StandardKey.Find] : []
            Accessible.name: i18nc("@label:textbox", "Search instances")
        }
    }

    actions: [
        Kirigami.Action {
            text: i18nc("@action:button", "New Instance…")
            icon.name: "list-add"
            onTriggered: page.app.openNewInstance()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Import…")
            icon.name: "document-import"
            onTriggered: page.app.openNewInstance("import")
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Sort")
            icon.name: "view-sort"
            SortAction {
                mode: "Name"
                text: i18nc("@action:inmenu sort by", "Name")
            }
            SortAction {
                mode: "LastLaunch"
                text: i18nc("@action:inmenu sort by", "Last Played")
            }
            SortAction {
                mode: "TotalTimePlayed"
                text: i18nc("@action:inmenu sort by", "Time Played")
            }
        },
        ViewAction {
            view: "grid"
            text: i18nc("@action:button", "Grid")
            icon.name: "view-list-icons"
        },
        ViewAction {
            view: "list"
            text: i18nc("@action:button", "List")
            icon.name: "view-list-details"
        }
    ]

    ListView {
        id: groupList
        model: page.groups
        spacing: Kirigami.Units.largeSpacing
        // A section's grid is all its delegates at once; rebuilding one
        // for another group would buy nothing.
        reuseItems: false

        delegate: InstanceGroup {
            required property string modelData
            width: ListView.view.width
            app: page.app
            group: modelData
            model: page.shell ? page.shell.sectionModel(modelData) : null
            view: page.app.libraryView
            // Instances in no group get no header, as in MultiMC and Prism:
            // a named "Ungrouped" read as one more group the user had made.
            showHeader: modelData.length > 0
            manageable: !!page.library && modelData.length > 0
            collapsed: !page.searching && modelData.length > 0 && page.collapsedGroups.indexOf(modelData) >= 0
            onToggleRequested: {
                if (page.library)
                    page.library.setGroupCollapsed(modelData, page.collapsedGroups.indexOf(modelData) < 0)
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: page.instanceCount === 0
            icon.name: page.searching ? "edit-find" : "applications-games-symbolic"
            text: page.searching ? i18n("No instances match “%1”", page.searchText) : i18n("No instances yet")
            explanation: page.searching
                         ? i18n("Check the spelling, or search for part of the name.")
                         : i18n("An instance is one Minecraft setup: a version, a mod loader and its mods.")
            helpfulAction: page.searching ? clearSearchAction : newInstanceAction
        }
    }

    Kirigami.Action {
        id: clearSearchAction
        text: i18nc("@action:button", "Clear Search")
        icon.name: "edit-clear"
        onTriggered: page.setSearch("")
    }
    Kirigami.Action {
        id: newInstanceAction
        text: i18nc("@action:button", "New Instance…")
        icon.name: "list-add"
        onTriggered: page.app.openNewInstance()
    }
}
