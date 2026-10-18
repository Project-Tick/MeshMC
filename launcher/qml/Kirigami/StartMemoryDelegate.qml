// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

import "memory.js" as Memory

/*
 * Starting memory: what the game reserves at launch, in the same 512 MB
 * steps as the maximum and never above it. Typed ("2 GB", "2048") or
 * stepped.
 */
SpinBoxDelegate {
    id: delegate

    required property var source
    property string key: "MinMemAlloc"
    property string maxKey: "MaxMemAlloc"
    readonly property int stored: delegate.source.number(delegate.key)

    label: i18nc("@label:spinbox", "Starting memory")
    description: i18n("Reserved when the game starts. Never more than the maximum.")
    from: Memory.floor
    to: Math.max(Memory.floor, delegate.source.number(delegate.maxKey))
    stepSize: Memory.step
    value: delegate.stored
    readonly property MemoryText memoryText: MemoryText {}
    textFromValue: (value, locale) => delegate.memoryText.describe(value)
    valueFromText: (text, locale) => {
        var mib = Memory.parse(text, delegate.from, delegate.to)
        return mib >= 0 ? mib : delegate.stored
    }
    onValueModified: {
        delegate.source.setValue(delegate.key, value)
        value = Qt.binding(() => delegate.stored)
    }
}
