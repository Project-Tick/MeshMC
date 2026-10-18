// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * The accounts games are launched with (AccountsController over the
 * AccountList): each player's face and name, which one is the default,
 * signing in with Microsoft (in the browser) or adding an offline account,
 * signing in again and removing. A Microsoft account opens its skin and
 * cape on a page of its own (AccountSkinPage.qml); an offline one has
 * neither, and says so.
 *
 * "The default account" is AccountsController.defaultAccountId, the account
 * AccountList::defaultAccount() launches with, not any row's own flag.
 */
FormCard.FormCardPage {
    id: page

    required property var app
    readonly property var controller: page.app.shell && page.app.shell.accountsController ? page.app.shell.accountsController : null
    readonly property var accounts: page.controller ? page.controller.accounts : null
    readonly property string defaultAccountId: page.controller ? page.controller.defaultAccountId : ""

    // Bumped whenever an account's data changes (a refresh, a new skin), so
    // the face urls change and QML fetches them again.
    property int revision: 0
    Connections {
        target: page.accounts
        ignoreUnknownSignals: true
        function onDataChanged() { page.revision++ }
    }
    Connections {
        target: page.controller
        ignoreUnknownSignals: true
        function onDefaultSkinsRevisionChanged() { page.revision++ }
    }

    function openSkin(row, accountId, name, isMSA) {
        if (isMSA)
            page.app.openAccountSkin(row, accountId, name)
        else
            page.app.notify(i18n("Skins and capes belong to a Microsoft account. Sign in with the one that owns Minecraft to change how you look."))
    }

    function askRemove(row, accountName) {
        removeDialog.row = row
        removeDialog.accountName = accountName
        removeDialog.open()
    }

    title: i18nc("@title", "Accounts")

    // The review snapshots' data has only offline accounts: a route with
    // "accounts-demo" in it opens the skin page on canned demo data instead
    // (AccountsController::skinDemoRequested()).
    Component.onCompleted: {
        if (page.controller && page.controller.skinDemoRequested)
            Qt.callLater(page.app.openAccountSkin, -1, "", i18n("Demo"))
    }

    actions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Sign In with Microsoft…")
            icon.name: "list-add-user"
            enabled: !!page.controller
            onTriggered: loginDialog.start()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Add Offline Account…")
            icon.name: "user-identity"
            enabled: !!page.controller
            onTriggered: offlineDialog.ask("")
        }
    ]

    // The default account's card; the row's own data is its delegate's, so
    // this makes one per account and only the default one shows.
    Repeater {
        model: page.accounts
        delegate: AccountHeaderCard {
            Layout.topMargin: Kirigami.Units.largeSpacing
            isDefault: page.defaultAccountId.length > 0 && accountId === page.defaultAccountId
            revision: page.revision
            onSkinRequested: page.openSkin(index, accountId, shownName, isMSA)
            onRefreshRequested: page.controller.refresh(index)
            onRemoveRequested: page.askRemove(index, shownName)
        }
    }
    // Accounts, but none to launch with: say what to do, gently.
    FormCard.FormCard {
        Layout.topMargin: Kirigami.Units.largeSpacing
        visible: accountList.count > 0 && page.defaultAccountId.length === 0
        FormCard.FormPlaceholderMessageDelegate {
            icon.name: "user-identity"
            text: i18n("No default account")
            explanation: i18n("Pick the account games should launch with. “Use as Default” is on its row below.")
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "All Accounts")
    }
    SeparatedCard {
        FormCard.FormPlaceholderMessageDelegate {
            visible: accountList.count === 0
            icon.name: "user-identity"
            text: i18n("No accounts yet")
            explanation: i18n("Sign in with the Microsoft account that owns Minecraft to play online.")
        }
        FormCard.FormButtonDelegate {
            visible: accountList.count === 0
            text: i18nc("@action:button", "Sign In with Microsoft…")
            icon.name: "list-add-user"
            enabled: !!page.controller
            onClicked: loginDialog.start()
        }
        Repeater {
            id: accountList
            model: page.accounts
            delegate: AccountDelegate {
                isDefault: page.defaultAccountId.length > 0 && accountId === page.defaultAccountId
                revision: page.revision
                onMakeDefaultRequested: page.controller.setDefault(index)
                onSkinRequested: page.openSkin(index, accountId, shownName, isMSA)
                onRefreshRequested: page.controller.refresh(index)
                onRemoveRequested: page.askRemove(index, shownName)
            }
        }
    }
    FormCard.FormSectionText {
        text: i18n("Games launch with the default account. Offline accounts need a Microsoft account that owns Minecraft.")
    }

    MicrosoftLoginDialog {
        id: loginDialog
        controller: page.controller
    }
    OfflineAccountDialog {
        id: offlineDialog
        controller: page.controller
    }
    Kirigami.PromptDialog {
        id: removeDialog
        property int row: -1
        property string accountName
        title: i18nc("@title:dialog", "Remove Account")
        subtitle: i18n("Remove “%1” from MeshMC? You can sign in again at any time.", removeDialog.accountName)
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Remove")
                icon.name: "list-remove-user"
                onTriggered: {
                    removeDialog.close()
                    page.controller.remove(removeDialog.row)
                }
            }
        ]
    }
}
