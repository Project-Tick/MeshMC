// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * A setting with a handful of named values, as a drop-down. `options` is
 * [{ value, label }]; the stored value picks the row, again whenever it
 * changes elsewhere.
 */
FormCard.FormComboBoxDelegate {
    id: delegate

    required property var source
    required property string key
    property var options: []
    readonly property string stored: delegate.source.string(delegate.key)

    // After the new value is stored.
    signal chosen(var value)

    model: delegate.options
    textRole: "label"
    valueRole: "value"

    // Set by a function, not bound: a click moves currentIndex itself.
    function sync() {
        currentIndex = delegate.options.findIndex(o => String(o.value) === delegate.stored)
    }
    onStoredChanged: sync()
    onOptionsChanged: sync()
    Component.onCompleted: sync()
    onActivated: {
        delegate.source.setValue(delegate.key, currentValue)
        delegate.chosen(currentValue)
        sync()
    }
}
