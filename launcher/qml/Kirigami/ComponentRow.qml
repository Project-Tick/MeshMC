// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD
import org.kde.kirigamiaddons.delegates as Delegates

/*
 * One component of the Version tab -- Minecraft, a mod loader, LWJGL, a
 * library patch -- with its version, a switch for those that can be turned
 * off, and a "More" button for the rest (also on right-click). A component
 * with a problem shows a warning in place of its icon.
 *
 * `model` carries PackProfile's roles (name, version, uid, problemSeverity,
 * isCustom, isEnabled, canDisable, ...).
 */
Delegates.RoundedItemDelegate {
    id: row

    required property var model
    required property int index
    // Whether components can be changed right now (not while the game runs).
    property bool unlocked: true
    // Whether the "More" menu would hold anything.
    property bool hasActions: true

    signal toggleRequested(string uid, bool enabled)
    signal menuRequested(Item parent, point position)

    readonly property string problem: row.model.problemSeverity || ""
    readonly property bool enabledComponent: row.model.isEnabled === true

    text: row.model.name
    contentItem: RowLayout {
        spacing: Kirigami.Units.smallSpacing

        KD.IconTitleSubtitle {
            Layout.fillWidth: true
            title: row.model.name
            subtitle: {
                var parts = [String(row.model.version)]
                if (row.model.isCustom)
                    parts.push(i18nc("@info a component edited by the user", "Custom"))
                if (!row.enabledComponent)
                    parts.push(i18nc("@info a component that is switched off", "Off"))
                return parts.join(" · ")
            }
            selected: row.highlighted
            font: row.font
            opacity: row.enabledComponent ? 1 : 0.6
            icon.name: row.problem === "error" ? "dialog-error"
                     : row.problem === "warning" ? "dialog-warning"
                     : row.model.uid === "net.minecraft" ? "applications-games-symbolic" : "package-symbolic"
            icon.width: Kirigami.Units.iconSizes.medium
            icon.height: Kirigami.Units.iconSizes.medium
        }
        QQC2.Switch {
            visible: row.model.canDisable === true
            enabled: row.unlocked
            checked: row.enabledComponent
            Accessible.name: i18nc("@option:check", "Enable %1", row.model.name)
            onToggled: {
                row.toggleRequested(row.model.uid, checked)
                checked = Qt.binding(() => row.enabledComponent)
            }
        }
        QQC2.ToolButton {
            id: moreButton
            // Keeps the switches in one column whether or not a row has a menu.
            opacity: row.hasActions ? 1 : 0
            enabled: row.hasActions
            icon.name: "overflow-menu"
            display: QQC2.AbstractButton.IconOnly
            text: i18nc("@action:button", "More")
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: row.menuRequested(moreButton, Qt.point(0, moreButton.height))
        }
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        enabled: row.hasActions
        onTapped: (eventPoint) => row.menuRequested(row, eventPoint.position)
    }
}
