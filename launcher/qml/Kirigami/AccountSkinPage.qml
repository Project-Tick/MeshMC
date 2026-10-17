// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * Skin and cape for one Microsoft account, pushed over Accounts: the player
 * (PlayerPreview, 3D where it can be), the arm width, the user's skins and
 * the game's nine default skins to pick from, and the account's capes.
 * Wear uploads the picked skin; Reset goes back to the default skin. Adding,
 * renaming and deleting skins is SkinLibraryDialogs.qml; what is picked
 * and what runs is SkinEditorState.qml.
 */
FormCard.FormCardPage {
    id: page

    required property var app
    property int row: -1
    property string accountId
    property string accountName

    readonly property var controller: page.app.shell && page.app.shell.accountsController ? page.app.shell.accountsController : null
    // Bumped whenever the account's data may have changed, so image urls
    // change and QML fetches them again.
    property int revision: 0

    readonly property SkinEditorState editor: SkinEditorState {
        controller: page.controller
        row: page.row
        accountId: page.accountId
        onCapeWorn: preview.showBack()
        onInfoChanged: page.revision++
    }
    readonly property var picked: page.editor.selected

    title: page.accountName.length > 0 ? i18nc("@title", "Skin and Cape for %1", page.accountName)
                                       : i18nc("@title", "Skin and Cape")

    Component.onCompleted: {
        page.editor.start()
        // Review renders: "accounts-demo=cape" or "=elytra" open on the
        // player's back, "=default" on Alex (AccountsController::skinDemoPose()).
        var pose = page.row === -1 && page.controller ? page.controller.skinDemoPose : ""
        if (pose === "cape" || pose === "elytra") {
            elytraBox.checked = pose === "elytra"
            Qt.callLater(preview.showBack)
        } else if (pose === "default" && page.editor.defaultList.length > 1) {
            page.editor.selectDefault(page.editor.defaultList[1])
        }
    }

    actions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Wear This Skin")
            icon.name: "dialog-ok-apply"
            enabled: page.editor.selectedReady && !page.editor.busy
            onTriggered: page.editor.wearSelected()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Add Skin")
            icon.name: "list-add"
            enabled: !!page.editor.skins && !page.editor.busy
            Kirigami.Action {
                text: i18nc("@action:inmenu", "From a File…")
                icon.name: "document-open"
                onTriggered: dialogs.addFromFile()
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "From a Link…")
                icon.name: "internet-services"
                onTriggered: dialogs.addFromLink()
            }
            Kirigami.Action {
                text: i18nc("@action:inmenu", "From a Player's Name…")
                icon.name: "user-identity"
                onTriggered: dialogs.addFromPlayer()
            }
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Reset to Default Skin")
            icon.name: "edit-undo"
            enabled: !page.editor.busy
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: page.editor.resetSkin()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Open Skins Folder")
            icon.name: "document-open-folder"
            enabled: !!page.editor.skins
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: page.editor.skins.openFolder()
        }
    ]

    // What the running change says, or what the last one ended with.
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 30
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Kirigami.Units.largeSpacing
        visible: page.editor.busy || page.editor.message.length > 0
        type: page.editor.messageIsError && !page.editor.busy ? Kirigami.MessageType.Error : Kirigami.MessageType.Information
        text: page.editor.busy ? (page.editor.watcher.status.length > 0 ? page.editor.watcher.status : i18n("Working…"))
                               : page.editor.message
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Player")
    }
    FormCard.FormCard {
        FormCard.AbstractFormDelegate {
            background: null
            focusPolicy: Qt.NoFocus
            contentItem: PlayerPreview {
                id: preview
                implicitHeight: Kirigami.Units.gridUnit * 16
                catAvailable: page.app.catAvailable
                skinTexture: page.picked ? page.picked.textureUrl || ""
                             : page.editor.info.hasSkin && page.accountId.length > 0
                               ? "image://accountface/skin/" + page.accountId + "?rev=" + page.revision : ""
                flatSource: page.picked ? page.picked.bodyUrl || ""
                            : page.accountId.length > 0 ? "image://accountface/body/" + page.accountId + "?rev=" + page.revision
                            : ""
                slim: page.editor.slim
                capeTexture: page.editor.info.currentCapeUrl || ""
                elytra: elytraBox.checked
            }
        }
        FormCard.FormCheckDelegate {
            id: elytraBox
            visible: preview.threeD
            text: i18nc("@option:check", "Show the elytra")
            description: i18n("Drag the player to turn it around.")
            onToggled: preview.showBack()
        }
        FormCard.FormDelegateSeparator {}
        ArmsRadio {
            slimArms: false
            text: i18nc("@option:radio arm width", "Classic arms")
        }
        ArmsRadio {
            slimArms: true
            text: i18nc("@option:radio arm width", "Slim arms")
        }
    }

    // Classic or Slim for the picked skin: checked follows the skin, not the
    // click, which only asks for the change (see SkinEditorState.qml).
    component ArmsRadio: FormCard.FormRadioDelegate {
        id: radio
        required property bool slimArms
        readonly property bool current: page.editor.slim === radio.slimArms
        enabled: !!page.picked && !page.editor.busy
        autoExclusive: false
        checked: radio.current
        onClicked: {
            page.editor.setSlim(radio.slimArms)
            checked = Qt.binding(() => radio.current)
        }
    }

    SkinLibraryCards {
        editor: page.editor
        onMenuRequested: (name, worn) => dialogs.openMenu(name, worn)
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Cape")
    }
    FormCard.FormCard {
        FormCard.AbstractFormDelegate {
            background: null
            focusPolicy: Qt.NoFocus
            contentItem: Flow {
                spacing: Kirigami.Units.smallSpacing
                CapeTile {
                    name: i18nc("@item cape choice", "No cape")
                    noCape: true
                    selected: (page.editor.info.currentCapeId || "").length === 0
                    enabled: !page.editor.busy
                    onClicked: if (!selected) page.editor.wearCape("")
                }
                Repeater {
                    model: page.editor.info.capes || []
                    delegate: CapeTile {
                        required property var modelData
                        name: modelData.alias
                        capeUrl: modelData.url
                        selected: modelData.id === page.editor.info.currentCapeId
                        enabled: !page.editor.busy
                        onClicked: if (!selected) page.editor.wearCape(modelData.id)
                    }
                }
            }
        }
    }

    SkinLibraryDialogs {
        id: dialogs
        editor: page.editor
    }
}
