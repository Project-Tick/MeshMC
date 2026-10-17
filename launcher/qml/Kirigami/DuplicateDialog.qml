// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

/*
 * "Duplicate…": a copy of one instance under a new name, in a group of its
 * own choosing, with or without its worlds and play time. The copy is
 * LibraryController.duplicate(); it runs in the background, and a passing
 * message says when it is ready.
 */
Kirigami.Dialog {
    id: dialog

    required property var app
    readonly property var library: dialog.app.shell && dialog.app.shell.library ? dialog.app.shell.library : null

    property string targetId
    // The copy in progress (a TaskWatcher), or null.
    property var watcher: null

    function start(id, name, group) {
        dialog.targetId = id
        nameField.text = i18n("%1 (copy)", name)
        groupField.model = dialog.app.instanceGroups().filter(g => g.length > 0)
        groupField.editText = group
        savesBox.checked = true
        playtimeBox.checked = true
        dialog.open()
        nameField.forceActiveFocus()
        nameField.selectAll()
    }

    title: i18nc("@title:dialog", "Duplicate Instance")
    padding: Kirigami.Units.largeSpacing
    preferredWidth: Kirigami.Units.gridUnit * 24
    standardButtons: Kirigami.Dialog.Cancel
    customFooterActions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Duplicate")
            icon.name: "edit-copy"
            enabled: nameField.text.trim().length > 0
            onTriggered: dialog.copy()
        }
    ]

    function copy() {
        var name = nameField.text.trim()
        var watcher = dialog.library
                ? dialog.library.duplicate(dialog.targetId, name, groupField.editText.trim(),
                                           savesBox.checked, playtimeBox.checked, "")
                : null
        dialog.close()
        if (!watcher) {
            dialog.app.notify(i18n("The instance could not be copied."))
            return
        }
        dialog.watcher = watcher
        dialog.app.notify(i18n("Copying “%1”…", name))
    }

    Kirigami.FormLayout {
        QQC2.TextField {
            id: nameField
            Kirigami.FormData.label: i18nc("@label:textbox", "Name:")
            onAccepted: if (text.trim().length > 0) dialog.copy()
        }
        QQC2.ComboBox {
            id: groupField
            Kirigami.FormData.label: i18nc("@label:listbox", "Group:")
            editable: true
        }
        QQC2.CheckBox {
            id: savesBox
            text: i18nc("@option:check", "Copy worlds")
        }
        QQC2.CheckBox {
            id: playtimeBox
            text: i18nc("@option:check", "Keep play time")
        }
    }

    Connections {
        target: dialog.watcher
        function onFinished(ok) {
            dialog.app.notify(ok ? i18n("The copy is ready in your library.")
                                 : i18n("The copy failed: %1", dialog.watcher.error))
            dialog.watcher = null
        }
    }
}
