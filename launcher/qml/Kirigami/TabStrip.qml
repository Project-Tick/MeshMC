// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

/*
 * The band a page's tabs stand in (an instance's tabs, Discover's
 * providers, Add Content's kinds): its contentItem is the tab bar, with
 * whatever goes beside it.
 *
 * The desktop style draws only the tabs themselves, which left a grey
 * patch as wide as the tabs and the rest of the row bare. The band is
 * Breeze's colour for a tab that is not the open one -- the window's, a
 * fifth of the way to its text's -- across the page's whole width, as
 * Breeze draws a document's tab bar: one strip, the open tab standing out.
 */
QQC2.ToolBar {
    id: strip

    // The tabs stand on the strip's bottom edge.
    bottomPadding: 0

    background: Rectangle {
        // The tabs' own colours, not the toolbar's header set.
        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        Kirigami.Theme.inherit: false
        readonly property color ink: Kirigami.Theme.textColor
        color: Qt.tint(Kirigami.Theme.backgroundColor, Qt.rgba(ink.r, ink.g, ink.b, 0.2))

        Kirigami.Separator {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }
    }
}
