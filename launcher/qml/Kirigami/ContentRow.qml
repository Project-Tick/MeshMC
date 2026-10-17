// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

/*
 * One installed file of the Content tab: its icon, name and version, and a
 * switch that turns it on or off. Right-click opens its menu.
 *
 * `model` carries ContentFilterModel's roles (key, name, version, enabled,
 * iconUrl, authors, homepage, ...).
 */
KD.SwitchSubtitleDelegate {
    id: row

    required property var model
    required property int index
    // Whether files of this kind can be switched right now (not while the
    // game has them open).
    property bool unlocked: true

    signal toggleRequested(string key, bool enabled)
    signal menuRequested(Item parent, point position)

    readonly property var authors: row.model.authors || []

    text: row.model.name || row.model.fileName || ""
    subtitle: {
        var parts = []
        if (row.model.version && String(row.model.version).length > 0)
            parts.push(String(row.model.version))
        if (row.authors.length > 0)
            parts.push(i18nc("@info %1 is a list of authors", "by %1", row.authors.join(", ")))
        return parts.join(" · ")
    }
    readonly property string iconUrl: row.model.iconUrl || ""
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

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (eventPoint) => row.menuRequested(row, eventPoint.position)
    }
}
