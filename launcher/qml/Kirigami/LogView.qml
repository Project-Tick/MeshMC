// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

/*
 * Log lines in a scrolling, monospaced view, in the View colours as an
 * editor has. `model` is a LogModel (roles `line` and `level`, coloured by
 * level) or a plain array of lines. Rows in `matchSet` get the neutral
 * highlight, `currentRow` the selection one.
 *
 * Without wrapping every line stays on one row and the view scrolls sideways
 * instead, which is what a stack trace wants.
 */
ListArea {
    id: view

    property var model: null
    property bool wrap: true
    // Keep the newest line in view as lines arrive.
    property bool follow: false
    // {row: true} for every line that matches the search.
    property var matchSet: ({})
    property int currentRow: -1
    // Shown instead of the lines when there are none; empty hides it.
    property string emptyText
    property string emptyExplanation

    // The user scrolled away from the end, or back to it.
    signal scrolledAway()
    signal reachedEnd()

    function positionAt(row) {
        lines.positionViewAtIndex(row, ListView.Center)
    }
    function positionAtEnd() {
        lines.positionViewAtEnd()
    }

    // MessageLevel::Enum, in the theme's colours.
    function levelColor(level) {
        switch (level) {
        case 2: case 8: case 9: return Kirigami.Theme.negativeTextColor   // StdErr, Error, Fatal
        case 7: return Kirigami.Theme.neutralTextColor                    // Warning
        case 3: return Kirigami.Theme.linkColor                           // MeshMC
        case 4: return Kirigami.Theme.disabledTextColor                   // Debug
        default: return Kirigami.Theme.textColor
        }
    }

    onFollowChanged: if (view.follow) lines.positionViewAtEnd()

    QQC2.ScrollBar.horizontal.policy: view.wrap ? QQC2.ScrollBar.AlwaysOff : QQC2.ScrollBar.AsNeeded

    ListView {
        id: lines
        clip: true
        model: view.model
        reuseItems: true
        boundsBehavior: Flickable.StopAtBounds
        // The widest line seen, for the sideways scroll range when lines do
        // not wrap.
        property real widest: 0
        contentWidth: view.wrap ? width : Math.max(width, widest + Kirigami.Units.largeSpacing * 2)
        flickableDirection: view.wrap ? Flickable.VerticalFlick : Flickable.HorizontalAndVerticalFlick
        onModelChanged: widest = 0
        onCountChanged: if (view.follow) Qt.callLater(lines.positionViewAtEnd)
        onMovementStarted: if (!atYEnd) view.scrolledAway()
        onAtYEndChanged: if (atYEnd && moving) view.reachedEnd()

        delegate: QQC2.Label {
            id: line
            required property int index
            // Roles of a LogModel (line, level), or `modelData` for a file's
            // lines.
            required property var model
            readonly property bool matched: view.matchSet[line.index] === true
            readonly property bool isCurrent: line.index === view.currentRow

            width: view.wrap ? ListView.view.width : implicitWidth
            leftPadding: Kirigami.Units.largeSpacing
            rightPadding: Kirigami.Units.largeSpacing
            text: line.model.line !== undefined ? line.model.line : line.model.modelData
            textFormat: Text.PlainText
            wrapMode: view.wrap ? Text.WrapAnywhere : Text.NoWrap
            font: Kirigami.Theme.fixedWidthFont
            color: line.isCurrent ? Kirigami.Theme.highlightedTextColor
                 : line.model.level !== undefined ? view.levelColor(line.model.level) : Kirigami.Theme.textColor
            background: Rectangle {
                visible: line.matched
                color: line.isCurrent ? Kirigami.Theme.highlightColor : Kirigami.Theme.neutralBackgroundColor
            }
            onImplicitWidthChanged: if (!view.wrap && implicitWidth > lines.widest) lines.widest = implicitWidth
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: view.emptyText.length > 0
            icon.name: "utilities-log-viewer"
            text: view.emptyText
            explanation: view.emptyExplanation
        }
    }
}
