// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * A tab made of FormCards, scrolling: what a FormCardPage is, as an item an
 * instance tab can be (the page around it is the instance's). Put headers,
 * cards and messages inside; they stack in one column.
 */
QQC2.ScrollView {
    id: tab

    default property alias cards: layout.data

    contentWidth: availableWidth
    // FormCardPage's background: between the window's and a view's.
    background: Rectangle {
        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        Kirigami.Theme.inherit: false
        Item {
            id: viewColors
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false
        }
        color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor,
                                                       viewColors.Kirigami.Theme.backgroundColor, 0.5)
    }

    ColumnLayout {
        id: layout
        width: tab.availableWidth
        spacing: 0
    }
}
