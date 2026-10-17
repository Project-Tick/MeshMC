// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * Puts text on the clipboard. Qt Quick has no clipboard of its own; a
 * hidden TextEdit's copy() is the usual way round that.
 */
TextEdit {
    id: clipboard

    visible: false
    textFormat: TextEdit.PlainText

    function copyText(text) {
        clipboard.text = text
        clipboard.selectAll()
        clipboard.copy()
        clipboard.text = ""
    }
}
