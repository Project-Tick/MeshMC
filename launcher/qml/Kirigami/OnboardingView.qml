// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * First run: the few choices MeshMC cannot make on its own, one step at a
 * time over the whole window, as Plasma's Welcome Center does it -- a
 * welcome, an account, Java and memory, and a way to start.
 *
 * It opens only when the shell still needs a setup step (setupSteps: Java,
 * by the rules the classic setup wizard had), and the account step is
 * there only while there is no account; both are fixed when the flow
 * opens, so it does not reshuffle as each choice is saved. Signing in
 * happens over the flow (MicrosoftLoginDialog), which carries on after.
 */
QQC2.Popup {
    id: flow

    required property var app
    readonly property var shell: flow.app.shell
    readonly property var accounts: flow.shell ? flow.shell.accountsController : null

    property var steps: []
    property int index: 0
    readonly property string step: flow.steps.length > 0 ? flow.steps[Math.min(flow.index, flow.steps.length - 1)] : ""
    readonly property bool last: flow.index >= flow.steps.length - 1
    // The account step asks to sign in or leave it for later, until there
    // is an account.
    readonly property bool waitingForAccount: flow.step === "account" && !!flow.shell && flow.shell.accountCount === 0

    // Opens the flow if a step is still needed, or with every step when
    // @p everyStep, on @p atStep if given (review snapshots). False when
    // there was nothing to do.
    function start(everyStep, atStep) {
        var needed = flow.shell && flow.shell.setupSteps ? flow.shell.setupSteps.slice() : []
        if (needed.length === 0 && !everyStep)
            return false
        var steps = ["welcome"]
        if (everyStep || flow.shell.accountCount === 0)
            steps.push("account")
        if (everyStep || needed.indexOf("java") >= 0) {
            steps.push("java")
            if (flow.shell && typeof flow.shell.detectJava === "function")
                flow.shell.detectJava()
        }
        steps.push("done")
        flow.steps = steps
        flow.index = Math.max(0, steps.indexOf(atStep || ""))
        flow.open()
        return true
    }
    function next() {
        // The shell checks again whether Java still needs setting up.
        if (flow.step === "java" && typeof flow.shell.finishSetupStep === "function")
            flow.shell.finishSetupStep("java")
        if (flow.last)
            flow.close()
        else
            flow.index++
    }
    // Leaves the flow for somewhere in the window: "newinstance" or a
    // navigate() path.
    function finishTo(target) {
        flow.close()
        if (target === "newinstance")
            flow.app.openNewInstance()
        else
            flow.app.navigate(target)
    }

    parent: QQC2.Overlay.overlay
    x: 0
    y: 0
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    modal: true
    dim: false
    closePolicy: QQC2.Popup.NoAutoClose
    padding: 0

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false
    background: Rectangle {
        color: Kirigami.Theme.backgroundColor
    }

    contentItem: ColumnLayout {
        spacing: 0

        QQC2.ScrollView {
            id: scroller
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

            OnboardingStep {
                width: scroller.availableWidth
                height: Math.max(implicitHeight, scroller.availableHeight)
                flow: flow
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }
        QQC2.ToolBar {
            Layout.fillWidth: true
            position: QQC2.ToolBar.Footer

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.Button {
                    text: i18nc("@action:button", "Back")
                    icon.name: flow.mirrored ? "go-next" : "go-previous"
                    enabled: flow.index > 0
                    onClicked: flow.index--
                }
                Item {
                    Layout.fillWidth: true
                }
                QQC2.PageIndicator {
                    count: flow.steps.length
                    currentIndex: flow.index
                    Accessible.name: i18nc("@info %1 step, %2 steps", "Step %1 of %2", flow.index + 1, flow.steps.length)
                }
                Item {
                    Layout.fillWidth: true
                }
                QQC2.Button {
                    visible: flow.waitingForAccount
                    text: i18nc("@action:button", "Later")
                    onClicked: flow.next()
                }
                QQC2.Button {
                    visible: !flow.waitingForAccount
                    text: flow.last ? i18nc("@action:button", "Start Playing") : i18nc("@action:button", "Next")
                    icon.name: flow.last ? "media-playback-start" : (flow.mirrored ? "go-previous" : "go-next")
                    highlighted: true
                    onClicked: flow.next()
                }
            }
        }
    }

    MicrosoftLoginDialog {
        id: loginDialog
        controller: flow.accounts
    }
    function signIn() {
        loginDialog.start()
    }
}
