// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * The first row of a group of one instance's settings: whether the group
 * follows the launcher's settings or the instance has its own. The rows
 * under it show the value in effect either way and are only editable while
 * `checked`. Turning it off drops the instance's own values, so the group
 * follows the launcher again.
 */
SettingSwitch {
    id: gate

    // The settings this group overrides, reset when the override goes.
    property var keys: []

    text: i18nc("@option:check", "Use custom settings for this instance")
    onSwitched: (on) => {
        if (on)
            return
        for (var i = 0; i < gate.keys.length; ++i)
            gate.source.reset(gate.keys[i])
    }
}
