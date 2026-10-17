// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

import "memory.js" as Memory

// An amount of memory in words: "512 MB" under a gigabyte, "6 GB" or
// "6.5 GB" from there (see memory.js).
QtObject {
    function describe(mib) {
        return Memory.inGib(mib) ? i18nc("@item amount of memory", "%1 GB", Memory.amount(mib))
                                 : i18nc("@item amount of memory", "%1 MB", Memory.amount(mib))
    }
}
