// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * The strip above an instance tab's list: whatever is put inside it (a
 * world picker, a search field) on the left, the tab's actions on the
 * right -- those that do not fit move into a "More" menu.
 */
QQC2.ToolBar {
    id: bar

    property alias actions: tools.actions
    default property alias leading: leadingRow.data

    // In the window's colours, as the open tab above it is: the tab and
    // its tools read as one piece, as on a tabbed panel.
    Kirigami.Theme.colorSet: Kirigami.Theme.Window

    contentItem: RowLayout {
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            id: leadingRow
            spacing: Kirigami.Units.smallSpacing
        }
        Kirigami.ActionToolBar {
            id: tools
            Layout.fillWidth: true
            alignment: Qt.AlignRight
        }
    }
}
