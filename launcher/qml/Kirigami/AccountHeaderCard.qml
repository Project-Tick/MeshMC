// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * The account games launch with, on top of the Accounts page: the player
 * standing full-length in their own skin (the flat front view the account
 * face provider draws, "body/<id>", so it also shows where Qt Quick3D does
 * not), the name, what kind of account it is, and the buttons for what
 * people come here to do -- change the skin, or manage the account.
 *
 * Made once per account row, like AccountDelegate, and visible only for the
 * default one, so the page needs no lookup of a row's data.
 */
FormCard.FormCard {
    id: card

    required property int index
    required property string profileName
    required property string name
    required property bool isMSA
    required property string stateKey
    required property string status
    required property string accountId

    property bool isDefault: false
    // Bumped by the page when skins may have changed, so the figure reloads.
    property int revision: 0

    readonly property string shownName: card.profileName.length > 0 ? card.profileName : card.name
    readonly property bool needsAttention: card.isMSA && card.stateKey !== "online" && card.status.length > 0

    signal skinRequested()
    signal refreshRequested()
    signal removeRequested()

    visible: card.isDefault

    FormCard.AbstractFormDelegate {
        background: null
        focusPolicy: Qt.NoFocus
        Accessible.name: card.shownName
        Accessible.description: i18nc("@info", "Default account")

        contentItem: RowLayout {
            spacing: Kirigami.Units.gridUnit

            // flatSource alone draws the figure; the skin url only says there
            // is one to draw (PlayerPreview shows an icon without it).
            PlayerPreview {
                Layout.preferredWidth: Kirigami.Units.gridUnit * 8
                Layout.preferredHeight: Kirigami.Units.gridUnit * 12
                Layout.alignment: Qt.AlignTop
                skinTexture: card.accountId.length > 0 ? "image://accountface/skin/" + card.accountId + "?rev=" + card.revision : ""
                flatSource: card.accountId.length > 0 ? "image://accountface/body/" + card.accountId + "?rev=" + card.revision : ""
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Kirigami.Icon {
                        source: "starred-symbolic"
                        implicitWidth: Kirigami.Units.iconSizes.small
                        implicitHeight: Kirigami.Units.iconSizes.small
                        color: Kirigami.Theme.positiveTextColor
                    }
                    QQC2.Label {
                        text: i18nc("@info status of the account games launch with", "Default account")
                        color: Kirigami.Theme.positiveTextColor
                    }
                }
                Kirigami.Heading {
                    Layout.fillWidth: true
                    level: 1
                    text: card.shownName
                    elide: Text.ElideRight
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: card.isMSA ? i18nc("@info account kind", "Microsoft account")
                                     : i18nc("@info account kind", "Offline account")
                    color: Kirigami.Theme.disabledTextColor
                    elide: Text.ElideRight
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    visible: card.needsAttention
                    text: card.status
                    color: card.stateKey === "errored" || card.stateKey === "gone" ? Kirigami.Theme.negativeTextColor
                         : Kirigami.Theme.neutralTextColor
                    wrapMode: Text.Wrap
                }

                Flow {
                    Layout.fillWidth: true
                    Layout.topMargin: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.smallSpacing
                    QQC2.Button {
                        text: i18nc("@action:button", "Change Skin…")
                        icon.name: "im-user"
                        onClicked: card.skinRequested()
                    }
                    QQC2.Button {
                        id: manageButton
                        text: i18nc("@action:button", "Manage")
                        icon.name: "configure"
                        onClicked: menu.popup(manageButton, 0, manageButton.height)
                    }
                }
            }
        }
    }

    AccountMenu {
        id: menu
        isMSA: card.isMSA
        onSkinRequested: card.skinRequested()
        onRefreshRequested: card.refreshRequested()
        onRemoveRequested: card.removeRequested()
    }
}
