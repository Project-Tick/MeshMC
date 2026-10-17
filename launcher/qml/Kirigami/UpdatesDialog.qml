// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * "Check for Updates": the check's progress, then what it found -- each
 * file from the version installed now to the newest one that runs on this
 * instance, with a link to what changed -- to tick and update (the widget
 * ModUpdateDialog). Closing it while it checks gives up on the check.
 */
Kirigami.Dialog {
    id: dialog

    // ContentUpdateController, and the check it shows (TaskWatcher).
    property var controller: null
    property var watcher: null
    // Started once Update is pressed: the download's TaskWatcher.
    signal updateStarted(var watcher)

    readonly property var updates: dialog.controller ? dialog.controller.updates : null
    readonly property bool checking: !!dialog.controller && dialog.controller.checking
    readonly property int count: dialog.updates ? dialog.updates.count : 0
    readonly property int checkedCount: dialog.updates ? dialog.updates.checkedCount : 0

    function show(watcher) {
        dialog.watcher = watcher
        dialog.open()
    }

    title: i18nc("@title:dialog", "Updates")
    preferredWidth: Kirigami.Units.gridUnit * 30
    maximumHeight: applicationWindow().height - Kirigami.Units.gridUnit * 4
    padding: 0
    standardButtons: Kirigami.Dialog.Close
    customFooterActions: [
        Kirigami.Action {
            text: i18ncp("@action:button", "Update %1", "Update %1", dialog.checkedCount)
            icon.name: "download"
            visible: !dialog.checking && dialog.count > 0
            enabled: dialog.checkedCount > 0
            onTriggered: {
                var watcher = dialog.controller.applyUpdates()
                dialog.close()
                if (watcher)
                    dialog.updateStarted(watcher)
            }
        }
    ]

    onClosed: {
        if (dialog.checking && dialog.controller)
            dialog.controller.cancelCheck()
        dialog.watcher = null
    }

    ColumnLayout {
        spacing: 0

        // While checking: what it is doing, and how far along.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            visible: dialog.checking
            spacing: Kirigami.Units.smallSpacing
            QQC2.Label {
                Layout.fillWidth: true
                text: dialog.watcher && dialog.watcher.status.length > 0 ? dialog.watcher.status
                                                                         : i18nc("@info:status", "Checking for updates…")
                elide: Text.ElideRight
            }
            LaunchProgressBar {
                Layout.fillWidth: true
                progress: dialog.watcher ? dialog.watcher.progress : -1
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            visible: !dialog.checking
            text: dialog.count === 0 ? i18n("Everything is up to date.")
                : i18n("Newer versions that run on this instance. Untick any you want to keep as they are.")
            wrapMode: Text.Wrap
        }

        QQC2.CheckBox {
            Layout.leftMargin: Kirigami.Units.largeSpacing
            visible: !dialog.checking && dialog.count > 1
            text: i18nc("@option:check", "All")
            tristate: true
            checkState: dialog.checkedCount === 0 ? Qt.Unchecked
                      : dialog.checkedCount === dialog.count ? Qt.Checked : Qt.PartiallyChecked
            nextCheckState: () => dialog.checkedCount === dialog.count ? Qt.Unchecked : Qt.Checked
            onClicked: dialog.updates.setAllChecked(checkState === Qt.Checked)
        }

        Repeater {
            model: dialog.checking ? null : dialog.updates
            delegate: QQC2.ItemDelegate {
                id: entry
                required property int index
                required property string name
                required property string provider
                required property string currentVersion
                required property string newVersion
                required property string changelog
                required property var model

                Layout.fillWidth: true
                text: entry.name
                onClicked: dialog.updates.setChecked(entry.index, !entry.model.checked)

                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing
                    QQC2.CheckBox {
                        checked: entry.model.checked
                        onToggled: {
                            dialog.updates.setChecked(entry.index, checked)
                            checked = Qt.binding(() => entry.model.checked)
                        }
                        Accessible.name: entry.name
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: 0
                        QQC2.Label {
                            Layout.fillWidth: true
                            text: entry.name
                            elide: Text.ElideRight
                        }
                        QQC2.Label {
                            Layout.fillWidth: true
                            text: [i18nc("@info %1 the version now, %2 the newer one", "%1 → %2",
                                         entry.currentVersion, entry.newVersion)]
                                  .concat(entry.provider.length > 0 ? [entry.provider] : []).join(" · ")
                            elide: Text.ElideRight
                            opacity: 0.7
                        }
                    }
                    QQC2.ToolButton {
                        visible: entry.changelog.length > 0
                        text: i18nc("@action:button", "What's New")
                        icon.name: "documentinfo"
                        display: QQC2.AbstractButton.IconOnly
                        onClicked: Qt.openUrlExternally(entry.changelog)
                        QQC2.ToolTip.text: text
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }
                }
            }
        }
    }
}
