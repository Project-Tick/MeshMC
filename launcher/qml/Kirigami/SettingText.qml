// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * A free-text setting. Saved when editing finishes (Enter, or leaving the
 * field), not on every key, so half-typed values never reach the config.
 */
FormCard.FormTextFieldDelegate {
    id: delegate

    required property var source
    required property string key
    readonly property string stored: delegate.source.string(delegate.key)

    // A long value (a path) shows its start, not its tail.
    function showStored() {
        text = stored
        cursorPosition = 0
    }
    onStoredChanged: if (!fieldActiveFocus) showStored()
    Component.onCompleted: showStored()
    onEditingFinished: if (text !== stored) delegate.source.setValue(delegate.key, text)
}
