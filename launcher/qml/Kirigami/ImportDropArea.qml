// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * A pack -- a .zip, an .mrpack or a link to one -- dropped anywhere on the
 * pages opens Import with it (Main.qml's openImport()), as the classic
 * window's drop did. It sits under the pages, so a drop area of theirs
 * takes its own drags first; while something is dragged over it, the pages
 * say what letting go will do.
 */
DropArea {
    id: area

    required property var app

    anchors.fill: parent
    z: -1
    keys: ["text/uri-list"]

    onDropped: (drop) => {
        if (!drop.hasUrls || drop.urls.length === 0)
            return
        area.app.openImport(drop.urls[0].toString())
        drop.accept(Qt.CopyAction)
    }

    Rectangle {
        parent: area.parent
        anchors.fill: parent
        z: 100
        visible: area.containsDrag
        Kirigami.Theme.colorSet: Kirigami.Theme.View
        Kirigami.Theme.inherit: false
        color: Qt.rgba(Kirigami.Theme.backgroundColor.r, Kirigami.Theme.backgroundColor.g,
                       Kirigami.Theme.backgroundColor.b, 0.92)

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            icon.name: "document-import"
            text: i18nc("@info", "Drop to Import")
            explanation: i18n("A .zip or .mrpack pack, or a link to one, becomes a new instance.")
        }
    }
}
