// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

/*
 * The user's own notes on an instance -- a seed, a server address, which
 * mods to update -- as one plain text box the size of the tab. Saved a
 * moment after typing stops, and at once when the tab is left or another
 * instance opens, always into the instance the text was typed for.
 */
Item {
    id: tab

    required property var app
    // InstanceDetails: its `notes` is read and written here.
    property var details: null

    // The instance the unsaved text belongs to: `details` may already be
    // the next instance by the time the text is saved.
    property var editing: null
    // Set while the text is filled in from the instance, so that is not
    // taken for typing.
    property bool loading: false
    readonly property int saveDelayMs: 800

    function save() {
        if (!saveTimer.running)
            return
        saveTimer.stop()
        if (tab.editing)
            tab.editing.notes = area.text
    }
    function load() {
        tab.loading = true
        area.text = tab.details ? tab.details.notes : ""
        tab.loading = false
        tab.editing = tab.details
    }

    onDetailsChanged: {
        tab.save()
        tab.load()
    }
    onVisibleChanged: if (!tab.visible) tab.save()
    Component.onCompleted: tab.load()
    Component.onDestruction: tab.save()

    // Changed elsewhere (a second window): shown, unless the user is in the
    // middle of typing here.
    Connections {
        target: tab.details
        ignoreUnknownSignals: true
        function onNotesChanged() {
            if (!saveTimer.running && area.text !== tab.details.notes)
                tab.load()
        }
    }

    Timer {
        id: saveTimer
        interval: tab.saveDelayMs
        onTriggered: {
            if (tab.editing)
                tab.editing.notes = area.text
        }
    }

    ListArea {
        anchors.fill: parent

        QQC2.TextArea {
            id: area
            objectName: "notesArea"
            placeholderText: i18n("Seed, server address, which mods to update…")
            wrapMode: TextEdit.Wrap
            selectByMouse: true
            background: null
            padding: Kirigami.Units.largeSpacing
            Accessible.name: i18nc("@label:textbox", "Notes")
            onTextChanged: if (!tab.loading) saveTimer.restart()
        }
    }
}
