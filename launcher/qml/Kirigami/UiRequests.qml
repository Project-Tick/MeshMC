// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * Answers the questions the core asks while it works -- "overwrite this?",
 * "which account?", "these mods must be downloaded by hand" -- as Kirigami
 * dialogs, and shows its "please wait" (UiHost::showBusy) as a modal one.
 * `host` is the shell's QmlUiHost: whenever it has a pending request
 * (`current`), the dialog opens on it, and closing it any way other than a
 * button counts as no. A plugin's file picker is the system's own file
 * dialog instead.
 *
 * Until this exists the core keeps asking through the classic dialogs
 * rather than waiting on a question nobody can see: it tells the host when
 * it can answer (setPresenterReady).
 */
Item {
    id: requests

    property var host: null
    readonly property var request: requests.host ? requests.host.current : null
    readonly property string kind: requests.request ? requests.request.kind : ""

    Component.onCompleted: if (requests.host) requests.host.setPresenterReady(true)
    Component.onDestruction: if (requests.host) requests.host.setPresenterReady(false)

    // The request the dialog was opened on, and whether a button has
    // answered it -- so closing never answers it twice, nor answers the
    // next one. An answered request leaves `current` (the host deletes
    // it), which is what closes the dialog or moves it on to the next.
    property var shownRequest: null
    property bool answered: false
    function answer(fn) {
        requests.answered = true
        fn()
    }

    onRequestChanged: {
        requests.shownRequest = requests.request
        requests.answered = false
        if (requests.kind === "filePicker") {
            if (dialog.opened)
                dialog.close()
            // Set fresh before every open: the dialog owns selectedFile
            // once shown, so a binding would only ever apply once.
            fileDialog.selectedFile = requests.request.filePickerMode === "save"
                    ? "file://" + requests.request.filePickerDefaultPath : ""
            fileDialog.open()
            return
        }
        if (requests.request)
            dialog.open()
        else if (dialog.opened)
            dialog.close()
    }

    Kirigami.PromptDialog {
        id: dialog

        readonly property bool wide: ["blockedMods", "untrustedMods", "update", "optionalMods"].indexOf(requests.kind) >= 0

        title: requests.request ? requests.request.title : ""
        dialogType: !requests.request ? Kirigami.PromptDialog.None
                  : requests.request.severity === "error" ? Kirigami.PromptDialog.Error
                  : requests.request.severity === "warning" ? Kirigami.PromptDialog.Warning
                  : Kirigami.PromptDialog.None
        preferredWidth: dialog.wide ? Kirigami.Units.gridUnit * 32 : Kirigami.Units.gridUnit * 24
        closePolicy: QQC2.Popup.CloseOnEscape
        standardButtons: Kirigami.Dialog.NoButton
        onClosed: {
            if (requests.request && requests.request === requests.shownRequest && !requests.answered
                    && requests.kind !== "filePicker") {
                requests.answered = true
                requests.request.reject()
            }
        }

        UiRequestBody {
            id: body
            request: requests.kind !== "filePicker" ? requests.request : null
            onSubmitted: {
                if (requests.kind === "text")
                    accept.trigger()
                else if (requests.kind === "pickAccount")
                    play.trigger()
            }
        }

        customFooterActions: [
            Kirigami.Action {
                id: accept
                visible: ["message", "confirm", "text", "blockedMods", "untrustedMods", "optionalMods"].indexOf(requests.kind) >= 0
                enabled: requests.kind === "blockedMods" ? body.allFound
                       : requests.kind === "untrustedMods" ? body.trusted
                       : requests.kind === "text" ? body.typedText.trim().length > 0 : true
                text: requests.request && requests.request.acceptLabel.length > 0 ? requests.request.acceptLabel
                    : requests.kind === "message" ? i18nc("@action:button", "OK") : i18nc("@action:button", "Continue")
                icon.name: "dialog-ok"
                onTriggered: {
                    var request = requests.request
                    requests.answer(() => requests.kind === "text" ? request.accept(body.typedText.trim())
                                                                     : request.accept())
                }
            },
            Kirigami.Action {
                id: play
                visible: requests.kind === "pickAccount"
                text: i18nc("@action:button", "Play")
                icon.name: "media-playback-start"
                onTriggered: {
                    var request = requests.request
                    requests.answer(() => request.chooseAccount(body.selectedAccount, body.makeDefault))
                }
            },
            Kirigami.Action {
                visible: requests.kind === "update"
                text: i18nc("@action:button", "Install Update")
                icon.name: "download"
                onTriggered: {
                    var request = requests.request
                    requests.answer(() => request.answerUpdate("install"))
                }
            },
            Kirigami.Action {
                visible: requests.kind === "update"
                text: i18nc("@action:button", "Skip This Version")
                onTriggered: {
                    var request = requests.request
                    requests.answer(() => request.answerUpdate("skip"))
                }
            },
            Kirigami.Action {
                visible: requests.kind === "update"
                text: i18nc("@action:button", "Later")
                onTriggered: {
                    var request = requests.request
                    requests.answer(() => request.answerUpdate("later"))
                }
            },
            // profileSetup: Create may fail without answering (a taken name,
            // a server error), so it does not close the dialog; the request
            // answers itself once the profile exists.
            Kirigami.Action {
                visible: requests.kind === "profileSetup"
                enabled: !!requests.request && requests.request.profileNameStatus === "available"
                         && !requests.request.profileSubmitting
                text: requests.request && requests.request.profileSubmitting
                      ? i18nc("@action:button", "Creating…") : i18nc("@action:button", "Create")
                icon.name: "list-add"
                onTriggered: requests.request.submitProfileName(body.profileName.trim())
            },
            Kirigami.Action {
                visible: requests.kind !== "message" && requests.kind !== "update"
                enabled: !(requests.request && requests.request.profileSubmitting)
                text: requests.request && requests.request.rejectLabel.length > 0 ? requests.request.rejectLabel
                                                                                  : i18nc("@action:button", "Cancel")
                icon.name: "dialog-cancel"
                onTriggered: {
                    var request = requests.request
                    requests.answer(() => request.reject())
                }
            }
        ]
    }

    // choose: one button per choice, the first the default -- added ahead
    // of the dialog's own footer actions, which then hide (see above).
    Instantiator {
        id: choices
        model: requests.kind === "choose" ? requests.request.actions : []
        delegate: Kirigami.Action {
            required property string modelData
            required property int index
            text: modelData
            onTriggered: {
                var request = requests.request
                requests.answer(() => request.choose(index))
            }
        }
        onObjectAdded: Qt.callLater(requests.syncChoices)
        onObjectRemoved: Qt.callLater(requests.syncChoices)
    }
    property var fixedFooterActions: []
    function syncChoices() {
        if (requests.fixedFooterActions.length === 0) {
            var fixed = []
            for (var j = 0; j < dialog.customFooterActions.length; ++j)
                fixed.push(dialog.customFooterActions[j])
            requests.fixedFooterActions = fixed
        }
        var list = []
        for (var i = 0; i < choices.count; ++i) {
            if (choices.objectAt(i))
                list.push(choices.objectAt(i))
        }
        dialog.customFooterActions = list.concat(requests.fixedFooterActions)
    }

    // A plugin's open/save dialog, the system's own.
    Dialogs.FileDialog {
        id: fileDialog
        readonly property var request: requests.kind === "filePicker" ? requests.request : null
        fileMode: fileDialog.request && fileDialog.request.filePickerMode === "save"
                  ? Dialogs.FileDialog.SaveFile : Dialogs.FileDialog.OpenFile
        nameFilters: fileDialog.request && fileDialog.request.filePickerFilter.length > 0
                     ? fileDialog.request.filePickerFilter.split(";;") : [i18n("All files (*)")]
        onAccepted: if (fileDialog.request) fileDialog.request.accept(selectedFile.toString())
        onRejected: if (fileDialog.request) fileDialog.request.reject()
    }

    // The core's "please wait": modal, with the one way out some waits
    // offer ("Play offline" while an account refreshes before a launch).
    Kirigami.Dialog {
        id: busyDialog
        readonly property bool busy: !!requests.host && requests.host.busy
        readonly property string skipLabel: requests.host && requests.host.busySkipLabel !== undefined
                                            ? requests.host.busySkipLabel : ""
        title: i18nc("@title:dialog", "Please Wait")
        padding: Kirigami.Units.largeSpacing
        preferredWidth: Kirigami.Units.gridUnit * 20
        closePolicy: QQC2.Popup.NoAutoClose
        showCloseButton: false
        standardButtons: Kirigami.Dialog.NoButton
        // Opened once the text and the way out, which change with `busy`,
        // have settled: opening on the change itself re-sizes the dialog
        // while it places itself.
        function openIfBusy() {
            if (busyDialog.busy)
                busyDialog.open()
        }
        onBusyChanged: busy ? Qt.callLater(busyDialog.openIfBusy) : close()

        RowLayout {
            spacing: Kirigami.Units.largeSpacing
            QQC2.BusyIndicator {
                running: busyDialog.busy
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: requests.host ? requests.host.busyText : ""
                wrapMode: Text.Wrap
            }
        }

        customFooterActions: [
            Kirigami.Action {
                visible: busyDialog.skipLabel.length > 0
                text: busyDialog.skipLabel
                onTriggered: requests.host.skipBusy()
            }
        ]
    }
}
