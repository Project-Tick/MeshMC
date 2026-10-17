// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * The step OnboardingView is on: its picture, its heading and what it asks,
 * then the choices themselves.
 */
ColumnLayout {
    id: page

    required property var flow
    readonly property string step: page.flow.step
    readonly property var shell: page.flow.shell
    readonly property bool signedIn: !!page.shell && page.shell.accountCount > 0

    readonly property var headings: ({
        welcome: i18nc("@title", "Welcome to MeshMC"),
        account: i18nc("@title", "Sign In to Play Online"),
        java: i18nc("@title", "Java and Memory"),
        done: i18nc("@title", "You're All Set")
    })
    readonly property var explanations: ({
        welcome: i18n("A Minecraft launcher for every version and mod loader. A couple of choices and you're playing."),
        account: i18n("Use the Microsoft account that owns Minecraft. You can also do this later, from Accounts."),
        java: i18n("Minecraft runs on Java. Pick one installed on this computer, or let MeshMC download the right one for each game."),
        done: i18n("Create an instance, or find a modpack in Discover.")
    })

    spacing: Kirigami.Units.largeSpacing

    Item {
        Layout.fillHeight: true
        Layout.minimumHeight: Kirigami.Units.gridUnit * 2
    }

    Image {
        Layout.alignment: Qt.AlignHCenter
        visible: page.step === "welcome"
        source: page.step === "welcome" ? "qrc:/org.projecttick.MeshMC.svg" : ""
        sourceSize.width: Kirigami.Units.iconSizes.enormous
        sourceSize.height: Kirigami.Units.iconSizes.enormous
        fillMode: Image.PreserveAspectFit
    }
    Kirigami.Icon {
        Layout.alignment: Qt.AlignHCenter
        visible: page.step !== "welcome"
        implicitWidth: Kirigami.Units.iconSizes.huge
        implicitHeight: Kirigami.Units.iconSizes.huge
        source: page.step === "account" ? "user-identity"
              : page.step === "java" ? "application-x-java"
              : "dialog-positive"
    }
    Kirigami.Heading {
        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.gridUnit
        Layout.rightMargin: Kirigami.Units.gridUnit
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        level: 1
        type: Kirigami.Heading.Type.Primary
        text: page.headings[page.step] || ""
    }
    QQC2.Label {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 28
        Layout.leftMargin: Kirigami.Units.gridUnit
        Layout.rightMargin: Kirigami.Units.gridUnit
        Layout.alignment: Qt.AlignHCenter
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: page.explanations[page.step] || ""
    }

    // Account
    QQC2.Button {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Kirigami.Units.largeSpacing
        visible: page.step === "account" && !page.signedIn
        enabled: !!page.flow.accounts
        text: i18nc("@action:button", "Sign In with Microsoft…")
        icon.name: "list-add-user"
        onClicked: page.flow.signIn()
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 24
        Layout.alignment: Qt.AlignHCenter
        visible: page.step === "account" && page.signedIn
        type: Kirigami.MessageType.Positive
        text: page.shell && page.shell.accountName ? i18n("Signed in as %1.", page.shell.accountName)
                                                   : i18n("Your account is ready.")
    }

    // Java
    OnboardingJavaStep {
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.largeSpacing
        visible: page.step === "java"
        shell: page.shell
    }

    // Done
    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Kirigami.Units.largeSpacing
        visible: page.step === "done"
        spacing: Kirigami.Units.largeSpacing
        QQC2.Button {
            text: i18nc("@action:button", "New Instance…")
            icon.name: "list-add"
            onClicked: page.flow.finishTo("newinstance")
        }
        QQC2.Button {
            text: i18nc("@action:button", "Browse Modpacks")
            icon.name: "internet-services"
            onClicked: page.flow.finishTo("discover")
        }
    }

    Item {
        Layout.fillHeight: true
        Layout.minimumHeight: Kirigami.Units.gridUnit * 2
    }
}
