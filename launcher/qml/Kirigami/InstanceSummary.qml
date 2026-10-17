// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

/*
 * What the library shows of one instance, in a grid card and a list row
 * alike: its icon, name and version, the launch's progress while it starts,
 * and buttons to play or stop it and to open its menu.
 *
 * `entry` is the delegate's model object, with InstanceList's roles
 * (instanceId, name, iconKey, group, isRunning, canLaunch, gameVersion,
 * loader, launchStatus, launchProgress).
 */
RowLayout {
    id: summary

    required property var app
    required property var entry
    property int iconSize: Kirigami.Units.iconSizes.large

    readonly property bool running: summary.entry.isRunning === true
    readonly property bool launching: (summary.entry.launchStatus || "").length > 0
    readonly property string versionLine: {
        var parts = [summary.entry.loader || "", summary.entry.gameVersion || ""].filter(s => s.length > 0)
        return parts.length > 0 ? parts.join(" ") : i18n("Minecraft")
    }
    // What InstanceDialogs.openMenu() needs to build the menu.
    readonly property var info: ({
        instanceId: summary.entry.instanceId,
        name: summary.entry.name,
        iconKey: summary.entry.iconKey,
        group: summary.entry.group,
        running: summary.running,
        canLaunch: summary.entry.canLaunch !== false
    })

    function openMenu(parent, position) {
        summary.app.dialogs.openMenu(summary.info, parent, position)
    }

    spacing: Kirigami.Units.largeSpacing

    Kirigami.Icon {
        Layout.preferredWidth: summary.iconSize
        Layout.preferredHeight: summary.iconSize
        source: "image://instanceicon/" + (summary.entry.iconKey && summary.entry.iconKey.length > 0
                                           ? summary.entry.iconKey : "default")
        // Instance icons are mostly pixel art.
        smooth: false
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing

        KD.TitleSubtitle {
            Layout.fillWidth: true
            title: summary.entry.name
            subtitle: summary.launching ? summary.entry.launchStatus
                    : summary.running ? i18nc("@info:status, %1 is the loader and version", "Running · %1", summary.versionLine)
                    : summary.versionLine
        }
        LaunchProgressBar {
            Layout.fillWidth: true
            visible: summary.launching
            progress: summary.entry.launchProgress
        }
    }

    QQC2.ToolButton {
        display: QQC2.AbstractButton.IconOnly
        text: summary.running || summary.launching ? i18nc("@action:button", "Stop") : i18nc("@action:button", "Play")
        icon.name: summary.running || summary.launching ? "media-playback-stop" : "media-playback-start"
        enabled: summary.running || summary.launching || summary.info.canLaunch
        onClicked: summary.running || summary.launching ? summary.app.stop(summary.entry.instanceId)
                                                         : summary.app.launch(summary.entry.instanceId)
        QQC2.ToolTip.text: text
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    }
    QQC2.ToolButton {
        id: menuButton
        display: QQC2.AbstractButton.IconOnly
        text: i18nc("@action:button", "More Actions")
        icon.name: "overflow-menu"
        onClicked: summary.openMenu(menuButton)
        QQC2.ToolTip.text: text
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    }
}
