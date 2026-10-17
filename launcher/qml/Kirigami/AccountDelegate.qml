// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * One account on the Accounts page: the player's face, name, what kind of
 * account it is and whether it still signs in, and its actions. The default
 * account -- the one games launch with -- says so; any other can be made
 * default in one click. A Microsoft account opens its skin and cape.
 */
FormCard.AbstractFormDelegate {
    id: delegate

    required property int index
    required property string profileName
    required property string name
    required property bool isMSA
    required property string stateKey
    required property string status
    required property string accountId

    property bool isDefault: false
    // Bumped by the page when skins may have changed, so the face reloads.
    property int revision: 0

    readonly property string shownName: delegate.profileName.length > 0 ? delegate.profileName : delegate.name

    signal makeDefaultRequested()
    signal skinRequested()
    signal refreshRequested()
    signal removeRequested()

    text: delegate.shownName
    onClicked: delegate.skinRequested()
    Accessible.description: description.text

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        Image {
            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
            Layout.preferredHeight: Kirigami.Units.iconSizes.medium
            source: delegate.accountId.length > 0
                    ? "image://accountface/" + delegate.accountId + "?rev=" + delegate.revision : ""
            sourceSize: Qt.size(Kirigami.Units.iconSizes.medium * 2, Kirigami.Units.iconSizes.medium * 2)
            smooth: false
            Accessible.ignored: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            QQC2.Label {
                Layout.fillWidth: true
                text: delegate.shownName
                elide: Text.ElideRight
                Accessible.ignored: true
            }
            QQC2.Label {
                id: description
                Layout.fillWidth: true
                text: {
                    var kind = delegate.isMSA ? i18nc("@info account kind", "Microsoft account")
                                              : i18nc("@info account kind", "Offline account")
                    var parts = [kind]
                    if (delegate.isDefault)
                        parts.push(i18nc("@info", "games launch with it"))
                    if (delegate.isMSA && delegate.stateKey !== "online" && delegate.status.length > 0)
                        parts.push(delegate.status)
                    return parts.join(" · ")
                }
                color: delegate.stateKey === "errored" || delegate.stateKey === "gone" ? Kirigami.Theme.negativeTextColor
                     : delegate.stateKey === "expired" ? Kirigami.Theme.neutralTextColor
                     : Kirigami.Theme.disabledTextColor
                elide: Text.ElideRight
                Accessible.ignored: true
            }
        }

        Kirigami.Icon {
            visible: delegate.isDefault
            source: "starred-symbolic"
            implicitWidth: Kirigami.Units.iconSizes.small
            implicitHeight: Kirigami.Units.iconSizes.small
            color: Kirigami.Theme.positiveTextColor
            QQC2.ToolTip.text: i18nc("@info:tooltip", "Default account")
            QQC2.ToolTip.visible: defaultHover.hovered
            HoverHandler { id: defaultHover }
        }
        QQC2.Button {
            visible: !delegate.isDefault
            text: i18nc("@action:button", "Use as Default")
            onClicked: delegate.makeDefaultRequested()
        }
        QQC2.ToolButton {
            id: moreButton
            icon.name: "overflow-menu"
            text: i18nc("@action:button", "More Actions")
            display: QQC2.AbstractButton.IconOnly
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: menu.popup(moreButton, 0, moreButton.height)
        }
    }

    QQC2.Menu {
        id: menu
        QQC2.MenuItem {
            text: i18nc("@action:inmenu", "Skin and Cape…")
            icon.name: "im-user"
            enabled: delegate.isMSA
            onTriggered: delegate.skinRequested()
        }
        QQC2.MenuItem {
            visible: delegate.isMSA
            height: visible ? implicitHeight : 0
            text: i18nc("@action:inmenu", "Sign In Again")
            icon.name: "view-refresh"
            onTriggered: delegate.refreshRequested()
        }
        QQC2.MenuSeparator {}
        QQC2.MenuItem {
            text: i18nc("@action:inmenu", "Remove…")
            icon.name: "list-remove-user"
            onTriggered: delegate.removeRequested()
        }
    }
}
