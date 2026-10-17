// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

.pragma library

/*
 * Memory amounts as the memory settings show and read them: the settings
 * keep MiB, the delegates say "512 MB" or "6 GB" and take "6 GB", "6g",
 * "6144" or "6144 MB" back. Every amount moves in steps of `step`.
 */

var step = 512
var floor = 512
var mibPerGib = 1024
// A bare number below this is meant in GB: nobody asks for 8 MB.
var bareGigabytesBelow = 128

// The largest maximum on a machine with @p systemMiB of memory.
function ceiling(systemMiB) {
    return Math.max(floor + step, Math.floor(systemMiB / step) * step)
}

// Whether @p mib reads in GB rather than MB.
function inGib(mib) {
    return mib >= mibPerGib
}

// The number part of @p mib as MemoryText says it: "512", "6", "6.5".
function amount(mib) {
    if (!inGib(mib))
        return String(mib)
    var decimals = mib % mibPerGib === 0 ? 0 : (mib % (mibPerGib / 2) === 0 ? 1 : 2)
    return Number(mib / mibPerGib).toLocaleString(Qt.locale(), "f", decimals)
}

// The MiB @p text stands for, snapped to a step and clamped to
// [@p from, @p to]; -1 for text that is no amount.
function parse(text, from, to) {
    var found = /^\s*([0-9]+(?:[.,][0-9]+)?)\s*([a-zA-Z]*)/.exec(text)
    if (!found)
        return -1
    var amount = parseFloat(found[1].replace(",", "."))
    var unit = found[2].toLowerCase().charAt(0)
    var mib = unit === "g" ? amount * mibPerGib
            : unit === "m" ? amount
            : amount < bareGigabytesBelow ? amount * mibPerGib : amount
    return Math.max(from, Math.min(to, Math.round(mib / step) * step))
}
