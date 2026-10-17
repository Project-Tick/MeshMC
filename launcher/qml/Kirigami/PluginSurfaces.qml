// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * Every plugin surface for one place in the UI (the settings, one
 * instance's page or settings), one after another, each under its title.
 * `model` is the shell's PluginSurfaceModel for that anchor
 * (shell.pluginSurfaces(anchor, instanceId)).
 *
 * A ColumnLayout that brings its own FormHeaders and FormCards: put it
 * straight into a FormCardPage. A surface whose document is a "section"
 * draws its own titled card; anything else is put in one, titled with the
 * surface when `showTitles`.
 */
ColumnLayout {
    id: root

    property var model: null
    property bool showTitles: true
    readonly property int count: repeater.count
    // The title a single surface would give a tab; "" with none.
    readonly property string firstTitle: repeater.count > 0 && repeater.itemAt(0) ? repeater.itemAt(0).title : ""

    Layout.fillWidth: true
    spacing: 0

    Repeater {
        id: repeater
        model: root.model
        delegate: ColumnLayout {
            id: surface
            required property string surfaceId
            required property string title
            required property var document
            required property int revision

            // revision is read so the node tree follows every patch.
            readonly property var rootNode: revision >= 0 && document ? (document.root || document) : ({})
            readonly property bool ownCard: rootNode.type === "section"

            Layout.fillWidth: true
            spacing: 0

            PluginNode {
                visible: surface.ownCard
                surfaceId: surface.surfaceId
                sink: root.model
                node: surface.ownCard ? surface.rootNode : ({})
            }

            FormCard.FormHeader {
                visible: !surface.ownCard && root.showTitles && surface.title.length > 0
                title: surface.title
            }
            FormCard.FormCard {
                visible: !surface.ownCard
                PluginNode {
                    inCard: true
                    surfaceId: surface.surfaceId
                    sink: root.model
                    node: surface.ownCard ? ({}) : surface.rootNode
                }
            }
        }
    }
}
