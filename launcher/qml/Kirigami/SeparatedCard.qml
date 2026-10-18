// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * A form card with a separator between its visible delegates, including
 * the ones a Repeater makes, where Kirigami Addons draws them itself
 * (autoSeparators). Kirigami Addons 1.13 and older -- what Fedora 43 and 44
 * ship -- have no such property, and a card that sets it there fails to
 * load, and with it the whole KDE interface; so it is set only where it
 * exists, and those versions show the delegates without lines between them.
 */
FormCard.FormCard {
    id: card

    Component.onCompleted: {
        if ("autoSeparators" in card)
            card.autoSeparators = true
    }
}
