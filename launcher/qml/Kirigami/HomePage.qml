// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * The start page: back to what was being played. The instance played last
 * as a big card with Play (HomeHero), the few played before it as artwork
 * cards, and the worlds played most recently across every instance, each
 * with the save's own icon. Everything comes from models the shell keeps
 * locally (recentModel, recentWorlds), so the page asks nothing of the
 * network. With nothing played yet, it says so and points at the library.
 */
Kirigami.ScrollablePage {
    id: page

    required property var app
    readonly property var shell: page.app.shell

    readonly property var recentModel: page.shell && page.shell.recentModel ? page.shell.recentModel : null
    readonly property var worldsModel: page.shell && page.shell.recentWorlds ? page.shell.recentWorlds : null
    readonly property int recentCount: page.recentModel && page.recentModel.count !== undefined ? page.recentModel.count : 0
    readonly property int instanceCount: page.app.instanceModel && page.app.instanceModel.count !== undefined
                                         ? page.app.instanceModel.count : 0
    readonly property int worldCount: worldRepeater.count

    // How many of each the page shows: a row of instances (as many as fit
    // across), two rows of worlds -- not the whole library.
    readonly property int recentShown: Math.max(1, recentCards.columns)
    readonly property int worldsShown: Math.max(1, worldCards.columns) * 2
    // The page's column stops growing here and centres.
    readonly property int columnWidth: Kirigami.Units.gridUnit * 60

    title: i18nc("@title", "Home")

    actions: [
        Kirigami.Action {
            text: i18nc("@action:button", "New Instance…")
            icon.name: "list-add"
            onTriggered: page.app.openNewInstance()
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing * 2

        // -- Continue Playing: the first row of recentModel ---------------
        Kirigami.Heading {
            Layout.fillWidth: true
            Layout.maximumWidth: page.columnWidth
            Layout.alignment: Qt.AlignHCenter
            visible: page.recentCount > 0
            level: 2
            text: i18nc("@title", "Continue Playing")
        }
        Repeater {
            model: page.recentModel
            delegate: Loader {
                id: heroLoader
                required property int index
                required property var model
                Layout.fillWidth: true
                Layout.maximumWidth: page.columnWidth
                Layout.alignment: Qt.AlignHCenter
                active: heroLoader.index === 0
                visible: active
                sourceComponent: HomeHero {
                    app: page.app
                    entry: heroLoader.model
                }
            }
        }

        // -- Recently Played: the next few ---------------------------------
        Kirigami.Heading {
            Layout.fillWidth: true
            Layout.maximumWidth: page.columnWidth
            Layout.alignment: Qt.AlignHCenter
            visible: page.recentCount > 1
            level: 2
            text: i18nc("@title", "Recently Played")
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: page.columnWidth
            Layout.alignment: Qt.AlignHCenter
            visible: page.recentCount > 1
            spacing: 0
            Kirigami.CardsLayout {
                id: recentCards
                // Every column as wide as the others, a short last row included.
                uniformCellWidths: true
                Layout.fillWidth: true
                maximumColumns: 4
                minimumColumnWidth: Kirigami.Units.gridUnit * 13
                maximumColumnWidth: Kirigami.Units.gridUnit * 19
                Repeater {
                    model: page.recentModel
                    delegate: Loader {
                        id: recentLoader
                        required property int index
                        required property var model
                        Layout.fillWidth: true
                        active: recentLoader.index >= 1 && recentLoader.index <= page.recentShown
                        visible: active
                        sourceComponent: InstanceCard {
                            app: page.app
                            model: recentLoader.model
                        }
                    }
                }
            }
        }

        // -- Recent Worlds -------------------------------------------------
        Kirigami.Heading {
            Layout.fillWidth: true
            Layout.maximumWidth: page.columnWidth
            Layout.alignment: Qt.AlignHCenter
            visible: page.worldCount > 0
            level: 2
            text: i18nc("@title", "Recent Worlds")
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: page.columnWidth
            Layout.alignment: Qt.AlignHCenter
            visible: page.worldCount > 0
            spacing: 0
            Kirigami.CardsLayout {
                id: worldCards
                // Every column as wide as the others, a short last row included.
                uniformCellWidths: true
                Layout.fillWidth: true
                maximumColumns: 3
                minimumColumnWidth: Kirigami.Units.gridUnit * 14
                maximumColumnWidth: Kirigami.Units.gridUnit * 20
                Repeater {
                    id: worldRepeater
                    model: page.worldsModel
                    delegate: Loader {
                        id: worldLoader
                        required property int index
                        required property var model
                        Layout.fillWidth: true
                        active: worldLoader.index < page.worldsShown
                        visible: active
                        sourceComponent: WorldCard {
                            app: page.app
                            worldName: worldLoader.model.worldName || ""
                            folderName: worldLoader.model.folderName || ""
                            iconUrl: worldLoader.model.iconUrl || ""
                            lastPlayed: worldLoader.model.lastPlayed
                            instanceId: worldLoader.model.instanceId || ""
                            instanceName: worldLoader.model.instanceName || ""
                        }
                    }
                }
            }
        }

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.gridUnit * 4
            visible: page.recentCount === 0
            // The app's own logo: this is the first thing a new user sees.
            icon.source: "qrc:/org.projecttick.MeshMC.svg"
            text: page.instanceCount === 0 ? i18n("Welcome to MeshMC") : i18n("Nothing played yet")
            explanation: page.instanceCount === 0
                         ? i18n("Create your first instance to start playing: one Minecraft setup, with its version, mod loader and mods.")
                         : i18n("What you play shows up here, ready to pick up again.")
            helpfulAction: page.instanceCount === 0 ? newInstanceAction : libraryAction
        }
    }

    Kirigami.Action {
        id: newInstanceAction
        text: i18nc("@action:button", "New Instance…")
        icon.name: "list-add"
        onTriggered: page.app.openNewInstance()
    }
    Kirigami.Action {
        id: libraryAction
        text: i18nc("@action:button", "Open Library")
        icon.name: "go-next"
        onTriggered: page.app.showSection("library")
    }
}
