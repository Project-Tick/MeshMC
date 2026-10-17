// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

/*
 * The desktop style opens a combo box's list as a menu holding every
 * entry, as tall as the window lets it: a list of a hundred loader or pack
 * versions ran from the window's top to its bottom. capPopup() keeps such
 * a list to a scrolling menu of a few rows under the box.
 */

// The height a long list stops at: about a dozen rows.
function maxPopupHeight(gridUnit) {
    return gridUnit * 20
}

// The combo box itself: @p item, or the one inside it (FormCard's
// FormComboBoxDelegate keeps its box to itself), or null.
function findComboBox(item) {
    if (!item)
        return null
    if (item.popup !== undefined && item.delegateModel !== undefined)
        return item
    var children = item.children || []
    for (var i = 0; i < children.length; ++i) {
        var found = findComboBox(children[i])
        if (found)
            return found
    }
    return null
}

// Caps the list of the combo box @p item is or holds at the height above.
// A box not found (another style, another FormCard) keeps its own list.
function capPopup(item, gridUnit) {
    var box = findComboBox(item)
    if (!box || !box.popup)
        return
    var popup = box.popup
    popup.height = Qt.binding(() => Math.min(popup.implicitHeight, maxPopupHeight(gridUnit)))
}
