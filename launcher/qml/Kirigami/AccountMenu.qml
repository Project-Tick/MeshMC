// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2

/*
 * What can be done to one account besides making it the default: its skin
 * and cape (a Microsoft account's), signing in again, and removing it.
 * Shared by the account's row (AccountDelegate.qml) and the default
 * account's header card (AccountHeaderCard.qml), which both open it from a
 * button.
 */
QQC2.Menu {
    id: menu

    property bool isMSA: false

    signal skinRequested()
    signal refreshRequested()
    signal removeRequested()

    QQC2.MenuItem {
        text: i18nc("@action:inmenu", "Skin and Cape…")
        icon.name: "im-user"
        enabled: menu.isMSA
        onTriggered: menu.skinRequested()
    }
    QQC2.MenuItem {
        visible: menu.isMSA
        height: visible ? implicitHeight : 0
        text: i18nc("@action:inmenu", "Sign In Again")
        icon.name: "view-refresh"
        onTriggered: menu.refreshRequested()
    }
    QQC2.MenuSeparator {}
    QQC2.MenuItem {
        text: i18nc("@action:inmenu", "Remove…")
        icon.name: "list-remove-user"
        onTriggered: menu.removeRequested()
    }
}
