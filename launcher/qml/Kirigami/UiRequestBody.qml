// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

/*
 * The body of one of the core's questions (a QmlUiRequest), whatever its
 * kind: the text, then what that kind asks for -- a line to type, the mods
 * to download by hand, the files to trust, a profile name, an account, the
 * optional mods of a pack, or an update's release notes. UiRequests.qml puts
 * it in a dialog and answers from the footer.
 */
ColumnLayout {
    id: body

    property var request: null
    readonly property string kind: body.request ? body.request.kind : ""

    // What the footer reads back.
    readonly property alias typedText: textAnswer.text
    readonly property alias profileName: profileField.text
    readonly property alias trusted: trustBox.checked
    readonly property alias makeDefault: makeDefaultBox.checked
    property int selectedAccount: 0
    readonly property bool allFound: {
        if (body.kind !== "blockedMods")
            return false
        var mods = body.request.blockedMods
        for (var i = 0; i < mods.length; ++i) {
            if (!mods[i].found)
                return false
        }
        return true
    }

    // Lists stop growing here and scroll instead.
    readonly property int listHeight: Kirigami.Units.gridUnit * 12

    signal submitted()

    // A new question: everything typed or ticked for the last one goes.
    onRequestChanged: {
        textAnswer.text = body.kind === "text" ? (body.request.value || "") : ""
        profileField.text = ""
        trustBox.checked = false
        makeDefaultBox.checked = false
        body.selectedAccount = 0
        if (body.kind === "untrustedMods")
            trustDelay.restart()
        if (body.kind === "text") {
            textAnswer.forceActiveFocus()
            textAnswer.selectAll()
        } else if (body.kind === "profileSetup") {
            profileField.forceActiveFocus()
        }
    }

    spacing: Kirigami.Units.largeSpacing

    QQC2.Label {
        Layout.fillWidth: true
        // Wrapped text still reports its whole unwrapped line as its
        // implicit width, which the dialog then grew to fit -- up to the
        // window's width for a long question. A preferred width of its own
        // leaves the dialog at the width it asks for (UiRequests.qml).
        Layout.preferredWidth: Kirigami.Units.gridUnit * 20
        // A plugin's modal brings its own text in its document.
        visible: text.length > 0
        text: body.request ? body.request.text : ""
        textFormat: Text.AutoText
        wrapMode: Text.Wrap
        onLinkActivated: (link) => Qt.openUrlExternally(link)
    }

    // text: one line, prefilled with the suggested answer.
    QQC2.TextField {
        id: textAnswer
        Layout.fillWidth: true
        visible: body.kind === "text"
        onAccepted: if (text.trim().length > 0) body.submitted()
    }

    // blockedMods: each file, found in Downloads yet or not.
    ColumnLayout {
        Layout.fillWidth: true
        visible: body.kind === "blockedMods"
        spacing: 0
        Repeater {
            model: body.kind === "blockedMods" ? body.request.blockedMods : []
            delegate: RowLayout {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                KD.IconTitleSubtitle {
                    Layout.fillWidth: true
                    title: modelData.fileName
                    subtitle: modelData.found ? i18nc("@info:status", "Found") : i18nc("@info:status", "Not downloaded yet")
                    icon.name: modelData.found ? "dialog-ok-apply" : "folder-download"
                }
                QQC2.Button {
                    visible: !modelData.found
                    text: i18nc("@action:button", "Download")
                    icon.name: "internet-services"
                    onClicked: body.request.openDownload(index)
                }
            }
        }
        QQC2.Button {
            text: i18nc("@action:button", "Check Again")
            icon.name: "view-refresh"
            onClicked: body.request.rescanDownloads()
        }
    }

    // untrustedMods: the files, and a deliberate tick before going on.
    ColumnLayout {
        Layout.fillWidth: true
        visible: body.kind === "untrustedMods"
        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(body.listHeight, untrustedList.contentHeight)
            ListView {
                id: untrustedList
                model: body.kind === "untrustedMods" ? body.request.untrustedModsFiles : []
                delegate: QQC2.Label {
                    required property string modelData
                    width: ListView.view.width
                    text: modelData
                    elide: Text.ElideMiddle
                }
            }
        }
        QQC2.CheckBox {
            id: trustBox
            text: i18nc("@option:check", "I trust these files")
            enabled: !trustDelay.running
            Timer {
                id: trustDelay
                interval: body.request ? body.request.confirmDelayMs : 0
            }
        }
    }

    // profileSetup: the name, checked as it is typed.
    ColumnLayout {
        Layout.fillWidth: true
        visible: body.kind === "profileSetup"
        QQC2.TextField {
            id: profileField
            Layout.fillWidth: true
            placeholderText: i18nc("@info:placeholder", "Username")
            enabled: !(body.request && body.request.profileSubmitting)
            onTextChanged: profileCheckDelay.restart()
            Timer {
                id: profileCheckDelay
                interval: Kirigami.Units.veryLongDuration
                onTriggered: if (body.kind === "profileSetup") body.request.checkProfileName(profileField.text.trim())
            }
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: body.kind === "profileSetup" && (body.request.profileNameStatus === "available"
                                                      || body.request.profileNameError.length > 0)
            type: body.kind === "profileSetup" && body.request.profileNameStatus === "available"
                  ? Kirigami.MessageType.Positive : Kirigami.MessageType.Warning
            text: body.kind !== "profileSetup" ? ""
                : body.request.profileNameStatus === "available" ? i18n("This name is available.")
                : body.request.profileNameError
        }
    }

    // pickAccount: one account to play as, maybe as the new default.
    ColumnLayout {
        Layout.fillWidth: true
        visible: body.kind === "pickAccount"
        spacing: 0
        Repeater {
            model: body.kind === "pickAccount" ? body.request.accountOptions : []
            delegate: KD.RadioSubtitleDelegate {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                text: modelData.name
                subtitle: modelData.kind === "Microsoft" ? i18nc("@info account type", "Microsoft account")
                                                         : i18nc("@info account type", "Offline account")
                icon.source: modelData.faceId ? "image://accountface/" + modelData.faceId : ""
                icon.name: modelData.faceId ? "" : "user-identity"
                checked: body.selectedAccount === index
                onToggled: body.selectedAccount = index
                onDoubleClicked: {
                    body.selectedAccount = index
                    body.submitted()
                }
            }
        }
        QQC2.CheckBox {
            id: makeDefaultBox
            text: i18nc("@option:check", "Make this the default account")
        }
    }

    // optionalMods: tick what to install along with the pack.
    ColumnLayout {
        Layout.fillWidth: true
        visible: body.kind === "optionalMods"
        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: body.listHeight
            ListView {
                id: modsList
                model: body.kind === "optionalMods" ? body.request.optionalMods : null
                delegate: KD.CheckSubtitleDelegate {
                    required property int index
                    required property string name
                    required property string description
                    required property bool selected
                    required property bool hidden
                    width: ListView.view.width
                    visible: !hidden
                    height: hidden ? 0 : implicitHeight
                    text: name
                    subtitle: description
                    checked: selected
                    onToggled: modsList.model.toggle(index)
                }
            }
        }
        RowLayout {
            QQC2.Button {
                text: i18nc("@action:button", "Recommended")
                onClicked: modsList.model.selectRecommended()
            }
            QQC2.Button {
                text: i18nc("@action:button select no mods", "None")
                onClicked: modsList.model.clearAll()
            }
        }
    }

    // update: from which version to which, and what changed.
    ColumnLayout {
        Layout.fillWidth: true
        visible: body.kind === "update"
        QQC2.Label {
            text: body.kind === "update"
                  ? i18nc("@info %1 and %2 are versions", "%1 → %2", body.request.updateInfo.currentVersion || "",
                          body.request.updateInfo.availableVersion || "")
                  : ""
            font.bold: true
        }
        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: body.listHeight
            visible: notes.text.length > 0
            QQC2.Label {
                id: notes
                width: parent.width
                text: body.kind === "update" ? body.request.updateInfo.releaseNotes || "" : ""
                textFormat: Text.MarkdownText
                wrapMode: Text.Wrap
                onLinkActivated: (link) => Qt.openUrlExternally(link)
            }
        }
    }

    // modal: a plugin's own dialog (a PluginNode document), which this
    // interface cannot draw yet; closing it backs out, as Cancel would.
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: body.kind === "modal"
        type: Kirigami.MessageType.Information
        text: i18n("This plugin's dialog is not available in this window yet.")
    }
}
