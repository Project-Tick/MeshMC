// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

import "memory.js" as Memory

/*
 * Maximum memory: a slider from the least the game can live with to the
 * memory this computer has, in 512 MB steps. The line under it says what
 * suits most modpacks, and warns once the amount eats into what the system
 * needs itself. The starting memory never ends up above it: lowering the
 * maximum below it pulls the starting amount down too.
 */
SliderDelegate {
    id: delegate

    required property var source
    property string key: "MaxMemAlloc"
    property string minKey: "MinMemAlloc"
    property int systemMiB: 8192

    readonly property int ceiling: Memory.ceiling(delegate.systemMiB)
    readonly property int stored: delegate.source.number(delegate.key)
    readonly property int shown: Math.round(delegate.value)
    // Past three quarters of the memory, the game competes with the system
    // and everything else that is open.
    readonly property real comfortableShare: 0.75
    readonly property bool tooMuch: delegate.shown > delegate.systemMiB * delegate.comfortableShare
    // What suits most modpacks: up to 8 GB, never past the comfortable
    // share, from half of that.
    readonly property int suitsHigh: Math.max(Memory.floor, Math.min(8192, Math.floor(delegate.ceiling * delegate.comfortableShare / Memory.step) * Memory.step))
    readonly property int suitsLow: Math.max(Memory.floor, Math.min(4096, Math.floor(delegate.suitsHigh / 2 / Memory.step) * Memory.step))

    readonly property MemoryText memoryText: MemoryText {}
    function describe(mib) {
        return delegate.memoryText.describe(mib)
    }
    function save() {
        var max = Math.max(Memory.floor, Math.min(delegate.ceiling, delegate.shown))
        delegate.source.setValue(delegate.key, max)
        if (delegate.source.number(delegate.minKey) > max)
            delegate.source.setValue(delegate.minKey, max)
    }

    label: i18nc("@label:slider", "Maximum memory")
    from: Memory.floor
    to: delegate.ceiling
    stepSize: Memory.step
    value: delegate.stored
    valueText: delegate.describe(delegate.shown)
    description: delegate.tooMuch
                 ? i18n("That is most of this computer's %1; the system and other programs may slow down.",
                        delegate.describe(delegate.systemMiB))
                 : i18n("This computer has %1. %2 to %3 suits most modpacks.", delegate.describe(delegate.systemMiB),
                        delegate.describe(delegate.suitsLow), delegate.describe(delegate.suitsHigh))
    // Only while it warns: the slider shows its trailing item whatever
    // that item's own visibility says.
    trailing: delegate.tooMuch ? warning : null
    readonly property Kirigami.Icon warning: Kirigami.Icon {
        source: "data-warning"
        implicitWidth: Kirigami.Units.iconSizes.small
        implicitHeight: Kirigami.Units.iconSizes.small
    }

    // Saved once the knob settles, not on every step of a drag.
    onMoved: saveDelay.restart()
    Timer {
        id: saveDelay
        interval: Kirigami.Units.humanMoment / 4
        onTriggered: {
            delegate.save()
            delegate.value = Qt.binding(() => delegate.stored)
        }
    }
    Component.onDestruction: if (saveDelay.running) delegate.save()
}
