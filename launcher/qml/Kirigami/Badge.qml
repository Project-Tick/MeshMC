// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

/*
 * A word in a small pill ("Snapshot", "Recommended"), drawn as Kirigami's
 * own Badge is: bold text on a fill tinted with the type's colour, outlined
 * in it. Kirigami.Badge is not in Kirigami 6.25, which Fedora 44 ships, and
 * a page that names it fails to load there, and with it the whole KDE
 * interface.
 */
QQC2.Label {
    id: badge

    // Information, Positive, Warning or Error.
    property int type: Kirigami.MessageType.Information

    readonly property color tone: badge.type === Kirigami.MessageType.Positive ? Kirigami.Theme.positiveTextColor
                                : badge.type === Kirigami.MessageType.Warning ? Kirigami.Theme.neutralTextColor
                                : badge.type === Kirigami.MessageType.Error ? Kirigami.Theme.negativeTextColor
                                : Kirigami.Theme.activeTextColor

    padding: Kirigami.Units.smallSpacing
    leftPadding: Kirigami.Units.smallSpacing * 2
    rightPadding: Kirigami.Units.smallSpacing * 2
    font.bold: true

    background: Rectangle {
        border.width: 1
        border.color: badge.tone
        color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, badge.tone, 0.2)
        radius: height / 2
    }
}
