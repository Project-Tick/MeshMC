// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components

/*
 * One group of the library: a section header that folds it, and its
 * instances as a grid of artwork cards or a list of rows. A named group's header
 * also has a menu to rename or delete the group.
 */
// Not "section": inside the ListView below that name is ListView.section.
ColumnLayout {
    id: groupSection

    required property var app
    // The group's name; "" for the instances in no group.
    property string group
    // The group's instances (QmlShell.sectionModel()).
    property var model: null
    // "grid" or "list".
    property string view: "grid"
    property bool showHeader: true
    property bool collapsed: false
    // Whether the header offers renaming and deleting the group.
    property bool manageable: false

    signal toggleRequested()

    spacing: Kirigami.Units.smallSpacing

    Kirigami.ListSectionHeader {
        Layout.fillWidth: true
        visible: groupSection.showHeader
        text: groupSection.group
        icon.name: groupSection.collapsed ? (groupSection.LayoutMirroring.enabled ? "arrow-left" : "arrow-right") : "arrow-down"
        Accessible.name: groupSection.collapsed ? i18nc("@action:button %1 is a group", "Show %1", groupSection.group)
                                           : i18nc("@action:button %1 is a group", "Hide %1", groupSection.group)
        onClicked: groupSection.toggleRequested()

        QQC2.ToolButton {
            id: groupMenuButton
            visible: groupSection.manageable
            display: QQC2.AbstractButton.IconOnly
            text: i18nc("@action:button", "Group Actions")
            icon.name: "overflow-menu"
            onClicked: groupMenu.popup(groupMenuButton, Qt.point(0, groupMenuButton.height))
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

            // A child of the button rather than of the layout, which
            // would give the (invisible) menu a row of its own.
            Components.ConvergentContextMenu {
                id: groupMenu
                Kirigami.Action {
                    text: i18nc("@action:inmenu", "Rename Group…")
                    icon.name: "edit-rename"
                    onTriggered: groupSection.app.dialogs.askRenameGroup(groupSection.group)
                }
                Kirigami.Action {
                    text: i18nc("@action:inmenu", "Delete Group…")
                    icon.name: "edit-delete"
                    onTriggered: groupSection.app.dialogs.askDeleteGroup(groupSection.group)
                }
            }
        }
    }

    Loader {
        Layout.fillWidth: true
        active: !groupSection.collapsed
        visible: active
        sourceComponent: groupSection.view === "list" ? listComponent : gridComponent
    }

    // Artwork cards, as many to a row as fit between the narrowest and the
    // widest card, filling the row.
    Component {
        id: gridComponent
        ColumnLayout {
            spacing: 0
            Kirigami.CardsLayout {
                // Every column as wide as the others, a short last row included.
                uniformCellWidths: true
                Layout.fillWidth: true
                maximumColumns: 0
                minimumColumnWidth: Kirigami.Units.gridUnit * 13
                maximumColumnWidth: Kirigami.Units.gridUnit * 19
                Repeater {
                    model: groupSection.model
                    delegate: InstanceCard {
                        Layout.fillWidth: true
                        app: groupSection.app
                    }
                }
            }
        }
    }

    Component {
        id: listComponent
        ListView {
            interactive: false
            // Nothing is current until the keyboard moves there: a current
            // row is drawn highlighted.
            currentIndex: -1
            implicitHeight: contentHeight
            model: groupSection.model
            delegate: InstanceRow {
                width: ListView.view.width
                app: groupSection.app
            }
        }
    }
}
