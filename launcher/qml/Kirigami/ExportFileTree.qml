// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * The files an export takes, as a tree to check and uncheck: the
 * InstanceExportController's `files`, which starts inside the instance
 * (never at the top of the disk) and remembers what was left out in the
 * instance's .packignore. Clicking a folder's row opens or closes it; its
 * box takes or leaves the folder whole.
 */
QQC2.ScrollView {
    id: view

    // InstanceExportController.
    required property var controller

    Component.onCompleted: view.background.visible = true
    QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

    TreeView {
        id: tree
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: view.controller ? view.controller.files : null

        // Only once there is a controller to ask for it.
        Binding {
            target: tree
            property: "rootIndex"
            when: !!view.controller
            value: view.controller ? view.controller.filesRoot : null
        }

        delegate: QQC2.ItemDelegate {
            id: node
            required property TreeView treeView
            required property bool expanded
            required property bool hasChildren
            required property int depth
            required property int row
            required property var model
            // The file's name: the `display` role, which a button has a
            // property of its own by that name for.
            readonly property string name: node.model.display
            required property int checkState
            required property bool isDir

            implicitWidth: node.treeView.width
            leftPadding: Kirigami.Units.smallSpacing + node.depth * Kirigami.Units.gridUnit
            Accessible.name: node.name
            onClicked: if (node.isDir) node.treeView.toggleExpanded(node.row)

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                // Folders only; a file keeps the room so names line up.
                Kirigami.Icon {
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                    opacity: node.isDir && node.hasChildren ? 1 : 0
                    source: node.expanded ? "go-down"
                          : node.mirrored ? "go-previous" : "go-next"
                }
                QQC2.CheckBox {
                    id: box
                    checkState: node.checkState
                    Accessible.name: node.name
                    // A partly checked folder becomes checked as a whole.
                    nextCheckState: function () {
                        return node.checkState === Qt.Checked ? Qt.Unchecked : Qt.Checked
                    }
                    // The model has the final say; the binding comes back
                    // so a later change (a parent folder unchecked) shows.
                    onToggled: {
                        node.model.checkState = box.checkState
                        box.checkState = Qt.binding(() => node.checkState)
                    }
                }
                Kirigami.Icon {
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                    source: node.isDir ? "folder" : "text-x-generic"
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: node.name
                    elide: Text.ElideRight
                    color: node.checkState === Qt.Unchecked ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
                }
            }
        }
    }
}
