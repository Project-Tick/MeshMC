// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

.pragma library

/*
 * Numbers the provider pages show: download counts as the catalogues
 * themselves write them ("12.3K", "4M"), in the locale's own digits.
 */

// "" for a count the provider does not give.
function compactNumber(value) {
    var n = Number(value)
    if (!(n >= 0))
        return ""
    if (n < 1000)
        return Number(n).toLocaleString(Qt.locale(), "f", 0)
    var units = ["K", "M", "B"]
    var unit = -1
    while (n >= 1000 && unit < units.length - 1) {
        n /= 1000
        unit++
    }
    return Number(n).toLocaleString(Qt.locale(), "f", n < 10 ? 1 : 0) + units[unit]
}

// The provider's ISO date as a Date, or null when it gives none.
function parseDate(text) {
    var ms = Date.parse(text || "")
    return ms > 0 ? new Date(ms) : null
}
