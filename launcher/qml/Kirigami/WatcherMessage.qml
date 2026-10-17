// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * Why a background task failed (a TaskWatcher from the instance's worlds,
 * backups or pack controllers): its own message when it has one, else
 * `failText`. Hidden while there is no watcher, or it is running or fine.
 * The task's progress is a ProgressFooter at the foot of the tab.
 */
Kirigami.InlineMessage {
    id: message

    property var watcher: null
    property string failText: i18n("That did not work.")

    visible: !!message.watcher && message.watcher.failed
    position: Kirigami.InlineMessage.Header
    type: Kirigami.MessageType.Error
    text: message.watcher ? (message.watcher.error || message.failText) : ""
}
