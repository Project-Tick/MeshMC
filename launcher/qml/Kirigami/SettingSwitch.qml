// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * An on/off setting, saved the moment it is flipped. `invert` is for a
 * setting stored as the opposite of what reads naturally ("Offer to update"
 * over SkipModpackUpdatePrompt).
 */
FormCard.FormSwitchDelegate {
    id: delegate

    required property var source
    required property string key
    property bool invert: false
    readonly property bool stored: delegate.invert !== delegate.source.bool(delegate.key)

    // After the new value is stored; `on` is what the switch now shows.
    signal switched(bool on)

    checked: delegate.stored
    onToggled: {
        delegate.source.setValue(delegate.key, delegate.invert ? !checked : checked)
        // Toggling drops the binding; put it back so a change made
        // elsewhere still shows here.
        checked = Qt.binding(() => delegate.stored)
        delegate.switched(checked)
    }
}
