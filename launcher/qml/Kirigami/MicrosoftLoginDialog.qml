// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * Signing in with Microsoft. The sign-in happens in the browser
 * (MicrosoftLoginController: an OAuth2 authorisation-code flow, whose page
 * opens by itself), so this says so while it waits, can open that page
 * again if the tab was lost, and says how it went. The login lives in C++,
 * so it goes on if this dialog closes; a success adds the account either
 * way.
 */
Kirigami.Dialog {
    id: dialog

    // AccountsController.
    property var controller: null
    // The sign-in in progress (MicrosoftLoginController), or null.
    property var login: null
    readonly property bool done: !!dialog.login && dialog.login.succeeded
    readonly property bool failed: !!dialog.login && dialog.login.failed

    function start() {
        if (!dialog.controller)
            return
        dialog.login = dialog.controller.loginMicrosoft()
        dialog.open()
    }

    title: i18nc("@title:dialog", "Sign In with Microsoft")
    padding: Kirigami.Units.largeSpacing
    preferredWidth: Kirigami.Units.gridUnit * 24
    closePolicy: QQC2.Popup.NoAutoClose
    showCloseButton: false
    standardButtons: Kirigami.Dialog.NoButton
    onClosed: dialog.login = null

    customFooterActions: [
        Kirigami.Action {
            visible: !dialog.done && !dialog.failed && !!dialog.login
                     && dialog.login.browserUrl.toString().length > 0
            text: i18nc("@action:button", "Open Browser Again")
            icon.name: "internet-services"
            onTriggered: dialog.login.openBrowser()
        },
        Kirigami.Action {
            visible: dialog.failed
            text: i18nc("@action:button", "Try Again")
            icon.name: "view-refresh"
            onTriggered: dialog.login = dialog.controller.loginMicrosoft()
        },
        Kirigami.Action {
            visible: dialog.done
            text: i18nc("@action:button", "Done")
            icon.name: "dialog-ok"
            onTriggered: dialog.close()
        },
        Kirigami.Action {
            visible: !dialog.done
            text: i18nc("@action:button", "Cancel")
            icon.name: "dialog-cancel"
            onTriggered: {
                if (dialog.login && dialog.login.running)
                    dialog.login.cancel()
                dialog.close()
            }
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        RowLayout {
            spacing: Kirigami.Units.largeSpacing
            QQC2.BusyIndicator {
                visible: !dialog.done && !dialog.failed
                running: visible
            }
            Kirigami.Icon {
                visible: dialog.done || dialog.failed
                source: dialog.done ? "dialog-positive" : "dialog-error"
                implicitWidth: Kirigami.Units.iconSizes.medium
                implicitHeight: Kirigami.Units.iconSizes.medium
            }
            QQC2.Label {
                Layout.fillWidth: true
                Layout.preferredWidth: Kirigami.Units.gridUnit * 18
                wrapMode: Text.Wrap
                text: dialog.done ? i18n("Signed in. You are ready to play.")
                    : dialog.failed ? (dialog.login.error || i18n("Signing in failed."))
                    : (dialog.login && dialog.login.status.length > 0 ? dialog.login.status
                                                                      : i18n("Continue in your browser…"))
            }
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.preferredWidth: Kirigami.Units.gridUnit * 20
            visible: !dialog.done && !dialog.failed
            wrapMode: Text.Wrap
            color: Kirigami.Theme.disabledTextColor
            text: i18n("Microsoft's sign-in page opened in your browser. Sign in there with the account that owns Minecraft; this updates by itself.")
        }
    }
}
