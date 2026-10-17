// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

/*
 * One installed file of the Content tab, laid out as KDE lists plugins: a
 * switch that turns it on or off, its icon, its name over who made it and
 * what it does, then its version and a button for its menu at the far end.
 * Right-click opens the same menu.
 *
 * A file that says nothing of itself (no authors, no description) shows
 * its file name instead: that is what tells two copies of a mod apart.
 *
 * `model` carries ContentFilterModel's roles (key, name, version, enabled,
 * iconUrl, fileName, description, authors, homepage, ...).
 */
QQC2.SwitchDelegate {
    id: row

    required property var model
    required property int index
    // Whether files of this kind can be switched right now (not while the
    // game has them open).
    property bool unlocked: true

    signal toggleRequested(string key, bool enabled)
    signal menuRequested(Item parent, point position)

    readonly property var authors: row.model.authors || []
    readonly property string version: row.model.version ? String(row.model.version) : ""
    // A description's first line: the rest would be cut off anyway.
    readonly property string description: String(row.model.description || "").trim().split("\n")[0]
    readonly property string subtitle: {
        var parts = []
        if (row.authors.length > 0)
            parts.push(i18nc("@info %1 is a list of authors", "by %1", row.authors.join(", ")))
        if (row.description.length > 0)
            parts.push(row.description)
        return parts.length > 0 ? parts.join(" · ") : (row.model.fileName || "")
    }
    readonly property string iconUrl: row.model.iconUrl || ""

    text: row.model.name || row.model.fileName || ""
    // The file's own icon when it declares one, else one of the launcher's
    // own block pictures, as for a mod's page without a logo.
    icon.source: row.iconUrl.length > 0 ? row.iconUrl : "image://instanceicon/gear"
    icon.width: Kirigami.Units.iconSizes.medium
    icon.height: Kirigami.Units.iconSizes.medium
    checked: row.model.enabled === true
    enabled: row.unlocked
    // The model says whether it is on; a toggle asks for the change, and
    // the row follows the model once the file is renamed.
    onToggled: {
        row.toggleRequested(row.model.key, checked)
        checked = Qt.binding(() => row.model.enabled === true)
    }

    QQC2.ToolTip.text: row.text + (row.subtitle.length > 0 ? "\n\n" + row.subtitle : "")
    QQC2.ToolTip.visible: row.hovered && label.truncated
    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        KD.IconTitleSubtitle {
            id: label
            Layout.fillWidth: true
            icon: icon.fromControlsIcon(row.icon)
            title: row.text
            subtitle: row.subtitle
            selected: row.highlighted || row.down
            font: row.font
        }
        // Versions line up in a column at the end of the rows.
        QQC2.Label {
            Layout.minimumWidth: Kirigami.Units.gridUnit * 4
            Layout.maximumWidth: Kirigami.Units.gridUnit * 10
            visible: row.version.length > 0
            text: row.version
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideMiddle
            opacity: 0.7
        }
        QQC2.ToolButton {
            id: moreButton
            icon.name: "overflow-menu"
            display: QQC2.AbstractButton.IconOnly
            text: i18nc("@action:button", "More Actions")
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: row.menuRequested(moreButton, Qt.point(0, moreButton.height))
        }
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (eventPoint) => row.menuRequested(row, eventPoint.position)
    }
}
