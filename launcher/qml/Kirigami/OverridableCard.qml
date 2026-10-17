// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * A card of settings that are launcher-wide in Settings and can be
 * overridden by one instance on its Settings tab. With `overrideKey` set
 * (the instance's gate, "OverrideWindow", ...) the card starts with the
 * "Use custom settings for this instance" switch, and the rows the card
 * adds below are `enabled: card.editable`: they show the value in effect
 * either way, and only change once the instance has its own. Without it,
 * the rows are always editable.
 */
FormCard.FormCard {
    id: card

    required property var source
    property string overrideKey: ""
    // What the override switch resets when it is turned off.
    property var overrideKeys: []
    property string overrideText: ""
    readonly property bool overridable: card.overrideKey.length > 0
    readonly property bool editable: !card.overridable || gate.checked

    OverrideSwitch {
        id: gate
        visible: card.overridable
        source: card.source
        key: card.overrideKey
        keys: card.overrideKeys
        text: card.overrideText.length > 0 ? card.overrideText
                                           : i18nc("@option:check", "Use custom settings for this instance")
    }
    FormCard.FormDelegateSeparator {
        visible: card.overridable
    }
}
