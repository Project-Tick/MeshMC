// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.coreaddons as KCoreAddons
import org.kde.kirigami as Kirigami

/*
 * When an instance was last played ("Last played: Yesterday at 21:04"), or
 * that it is running now, or that its last run crashed. `lastLaunch` is
 * InstanceList's role of that name: milliseconds since the epoch, 0 for
 * never.
 */
QQC2.Label {
    id: label

    property var lastLaunch: 0
    property bool running: false
    property bool crashed: false

    readonly property real lastLaunchMs: Number(label.lastLaunch) || 0

    text: label.running ? i18nc("@info:status", "Running")
        : label.crashed ? i18nc("@info:status", "Crashed last time")
        : label.lastLaunchMs > 0
          ? i18nc("@info %1 is a relative date and time such as 'Yesterday at 21:04'", "Last played: %1",
                  KCoreAddons.Format.formatRelativeDateTime(new Date(label.lastLaunchMs), Locale.ShortFormat))
          : i18nc("@info", "Never played")
    color: label.crashed && !label.running ? Kirigami.Theme.negativeTextColor
         : label.running ? Kirigami.Theme.positiveTextColor
         : Kirigami.Theme.textColor
    opacity: label.crashed || label.running ? 1 : 0.7
    elide: Text.ElideRight
}
