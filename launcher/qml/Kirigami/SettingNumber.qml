// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * A whole-number setting within bounds, with an optional unit after the
 * number ("px", "MiB"), saved whenever the value is changed.
 */
FormCard.FormSpinBoxDelegate {
    id: delegate

    required property var source
    required property string key
    property string suffix: ""
    readonly property int stored: delegate.source.number(delegate.key)

    value: delegate.stored
    textFromValue: (value, locale) => {
        var number = Number(value).toLocaleString(locale, "f", 0)
        return delegate.suffix.length > 0 ? i18nc("@item:spinbox number and its unit", "%1 %2", number, delegate.suffix)
                                          : number
    }
    valueFromText: (text, locale) => {
        var digits = text.replace(/[^0-9]/g, "")
        return digits.length > 0 ? parseInt(digits, 10) : delegate.stored
    }
    onValueModified: {
        delegate.source.setValue(delegate.key, value)
        value = Qt.binding(() => delegate.stored)
    }
}
