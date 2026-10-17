// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * "Remove X?" and its kind: a warning that names what happens (`subtitle`),
 * Cancel, and the one action to confirm (`actionText`, Remove by default).
 * `confirmed` fires after the dialog has closed.
 */
Kirigami.PromptDialog {
    id: dialog

    property string actionText: i18nc("@action:button", "Remove")
    property string actionIcon: "edit-delete"

    signal confirmed()

    dialogType: Kirigami.PromptDialog.Warning
    standardButtons: Kirigami.Dialog.Cancel
    customFooterActions: [
        Kirigami.Action {
            text: dialog.actionText
            icon.name: dialog.actionIcon
            onTriggered: {
                dialog.close()
                dialog.confirmed()
            }
        }
    ]
}
