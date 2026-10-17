// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components

/*
 * The dialogs behind InstanceActions -- rename, change icon, change group,
 * duplicate, export, create shortcut and delete -- the instance context
 * menu, and renaming or deleting a group, in one place so the library and
 * the instance page open the same ones. Main.qml has one, as `app.dialogs`.
 */
Item {
    id: dialogs

    required property var app
    readonly property var shell: dialogs.app.shell
    readonly property var library: dialogs.shell && dialogs.shell.library ? dialogs.shell.library : null

    // The context menu for one instance, at @p position in @p parent (under
    // the pointer), or under @p parent when no position is given (a ⋯
    // button). @p info is {instanceId, name, iconKey, group, running,
    // canLaunch}, as the library's delegates have them.
    function openMenu(info, parent, position) {
        var menu = menuComponent.createObject(dialogs, { info: info })
        if (position)
            menu.popup(parent, position)
        else
            menu.popup(parent, Qt.point(0, parent.height))
    }
    Component {
        id: menuComponent
        Components.ConvergentContextMenu {
            id: menu
            required property var info
            readonly property InstanceActions instanceActions: InstanceActions {
                app: dialogs.app
                instanceId: menu.info.instanceId
                name: menu.info.name
                iconKey: menu.info.iconKey
                group: menu.info.group
                running: menu.info.running
                canLaunch: menu.info.canLaunch
            }
            actions: menu.instanceActions.menuActions()
            onClosed: menu.destroy()
        }
    }

    function askRename(id, name) {
        renameDialog.targetId = id
        renameField.text = name
        renameDialog.failed = false
        renameDialog.open()
        renameField.forceActiveFocus()
        renameField.selectAll()
    }
    function askGroup(id, group) {
        groupDialog.targetId = id
        groupField.model = dialogs.app.instanceGroups().filter(g => g.length > 0)
        groupField.editText = group
        groupDialog.open()
        groupField.forceActiveFocus()
    }
    function askIcon(id, iconKey) {
        iconPicker.targetId = id
        iconPicker.current = iconKey
        iconPicker.open()
    }
    function askDelete(id, name) {
        deleteDialog.targetId = id
        deleteDialog.targetName = name
        deleteDialog.open()
    }
    function askRenameGroup(group) {
        groupRenameDialog.group = group
        groupRenameField.text = group
        groupRenameDialog.open()
        groupRenameField.forceActiveFocus()
        groupRenameField.selectAll()
    }
    function askDeleteGroup(group) {
        groupDeleteDialog.group = group
        groupDeleteDialog.open()
    }
    function duplicate(id, name, group) {
        duplicateDialog.start(id, name, group)
    }
    function createShortcut(id) {
        var controller = dialogs.shell && typeof dialogs.shell.shortcutController === "function"
                ? dialogs.shell.shortcutController(id) : null
        if (controller)
            shortcutDialog.start(controller)
    }
    // @p format ("zip", "modrinth", "curseforge") is optional: the dialog
    // opens on the instance's last one otherwise (a zip at first).
    function exportInstance(id, format) {
        var controller = dialogs.shell && typeof dialogs.shell.exportController === "function"
                ? dialogs.shell.exportController(id) : null
        if (controller)
            exportDialog.start(controller, format || "")
    }

    Kirigami.PromptDialog {
        id: renameDialog
        property string targetId
        property bool failed: false
        title: i18nc("@title:dialog", "Rename Instance")
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        onAccepted: {
            // Kept open with the reason when the name cannot be used.
            if (!dialogs.shell.renameInstance(targetId, renameField.text)) {
                failed = true
                open()
            }
        }

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing
            QQC2.TextField {
                id: renameField
                Layout.fillWidth: true
                Accessible.name: i18nc("@label:textbox", "Name")
                onAccepted: renameDialog.accept()
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                visible: renameDialog.failed
                type: Kirigami.MessageType.Error
                text: i18n("That name cannot be used.")
            }
        }
    }

    Kirigami.PromptDialog {
        id: groupDialog
        property string targetId
        title: i18nc("@title:dialog", "Change Group")
        subtitle: i18n("Type a new group or pick an existing one. Leave it empty for no group.")
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        onAccepted: dialogs.shell.setInstanceGroup(targetId, groupField.editText.trim())

        QQC2.ComboBox {
            id: groupField
            editable: true
            Accessible.name: i18nc("@label:listbox", "Group")
        }
    }

    Kirigami.PromptDialog {
        id: deleteDialog
        property string targetId
        property string targetName
        title: i18nc("@title:dialog", "Delete Instance")
        subtitle: i18n("Delete “%1”? Its folder, with its worlds, mods and screenshots, is removed.", targetName)
        dialogType: Kirigami.PromptDialog.Warning
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Delete")
                icon.name: "edit-delete"
                onTriggered: {
                    deleteDialog.close()
                    dialogs.remove(deleteDialog.targetId)
                }
            }
        ]
    }
    function remove(id) {
        if (!dialogs.shell.deleteInstance(id)) {
            dialogs.app.notify(i18n("The instance could not be deleted. Is it running?"))
            return
        }
        // Deleted from its own page: there is no page left to show.
        if (dialogs.app.openedInstanceId === id)
            dialogs.app.showSection("library")
        // Undo only when it went to the trash; a platform without one
        // deleted it for good.
        var library = dialogs.library
        if (library && library.canUndoDelete) {
            var trashedName = library.lastDeletedName
            dialogs.app.notify(i18n("Instance deleted."), i18nc("@action:button", "Undo"), function () {
                if (!library.undoDelete())
                    dialogs.app.notify(i18n("“%1” could not be put back. It is still in the trash.", trashedName))
            })
        } else {
            dialogs.app.notify(i18n("Instance deleted."))
        }
    }

    Kirigami.PromptDialog {
        id: groupRenameDialog
        property string group
        title: i18nc("@title:dialog", "Rename Group")
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        onAccepted: if (dialogs.library) dialogs.library.renameGroup(group, groupRenameField.text)

        QQC2.TextField {
            id: groupRenameField
            Accessible.name: i18nc("@label:textbox", "Name")
            onAccepted: groupRenameDialog.accept()
        }
    }

    // Nothing is lost: the instances stay, in no group.
    Kirigami.PromptDialog {
        id: groupDeleteDialog
        property string group
        title: i18nc("@title:dialog", "Delete Group")
        subtitle: i18n("Delete the group “%1”? Its instances stay in your library, in no group.", group)
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Delete Group")
                icon.name: "edit-delete"
                onTriggered: {
                    groupDeleteDialog.close()
                    if (dialogs.library)
                        dialogs.library.deleteGroup(groupDeleteDialog.group)
                }
            }
        ]
    }

    IconPickerDialog {
        id: iconPicker
        property string targetId
        shell: dialogs.shell
        onPicked: (key) => dialogs.shell.setInstanceIcon(targetId, key)
    }

    DuplicateDialog {
        id: duplicateDialog
        app: dialogs.app
    }

    ShortcutDialog {
        id: shortcutDialog
        app: dialogs.app
    }

    ExportInstanceDialog {
        id: exportDialog
        app: dialogs.app
    }
}
