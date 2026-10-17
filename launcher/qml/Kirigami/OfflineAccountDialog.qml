// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * Adding an offline account: the name shown in game. Offline accounts need
 * a Microsoft account that owns the game first, and a name no other offline
 * account uses -- AccountsController.addOffline() refuses otherwise, and the
 * dialog stays open saying so.
 */
TextPromptDialog {
    id: dialog

    // AccountsController.
    property var controller: null

    title: i18nc("@title:dialog", "Add Offline Account")
    subtitle: i18n("The name shown in game. Offline accounts cannot join online-mode servers.")
    placeholderText: i18nc("@info:placeholder", "Player name")
    confirmText: i18nc("@action:button", "Add Account")
    onSubmitted: (text) => {
        if (dialog.controller && dialog.controller.addOffline(text))
            dialog.close()
        else
            dialog.error = i18n("Offline accounts need a Microsoft account that owns the game first, and a name no other offline account uses.")
    }
}
