// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * A page title with a search field beside it, for a page's titleDelegate,
 * that searches a catalogue: typing searches once it pauses, Enter at once,
 * so a remote search is not sent per keystroke. `text` is the page's own
 * search words, which a link or the dev route can change too.
 */
RowLayout {
    id: heading

    property string title
    property string text
    property alias placeholderText: field.placeholderText
    property alias fieldEnabled: field.enabled
    // Whether Ctrl+F reaches this field (only the page on screen).
    property bool current: false
    // The words changed (the page keeps them in `text`), and when to search.
    signal edited(string text)
    signal searchRequested(string text)

    spacing: Kirigami.Units.largeSpacing

    Kirigami.Heading {
        text: heading.title
        elide: Text.ElideRight
        Layout.minimumWidth: 0
    }
    Kirigami.SearchField {
        id: field
        Layout.preferredWidth: Kirigami.Units.gridUnit * 16
        Layout.minimumWidth: Kirigami.Units.gridUnit * 6
        focusSequences: heading.current ? [StandardKey.Find] : []
        text: heading.text
        autoAccept: false
        onTextChanged: {
            if (text === heading.text)
                return
            heading.edited(text)
            pause.restart()
        }
        onAccepted: {
            pause.stop()
            heading.searchRequested(field.text)
        }
        Accessible.name: heading.placeholderText
    }
    Timer {
        id: pause
        interval: Kirigami.Units.humanMoment / 6
        onTriggered: heading.searchRequested(field.text)
    }
}
