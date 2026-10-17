// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import Qt.labs.qmlmodels
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

/*
 * What an install brings along, before anything is downloaded (the widget
 * DownloadSummaryDialog). Two shapes, from the review's primaryCount:
 *   - a picked project (Add Content): always installed, so its row has no
 *     check box; what it needs is listed under it, each one optional, and
 *     one already installed at another version starts unticked;
 *   - missing dependencies (Verify Dependencies): only what the installed
 *     mods need and do not have, all optional.
 * Dependencies no provider had a compatible version of are named in a
 * warning underneath, since things may not work without them.
 *
 * Reviews come one at a time: show() while one is open queues the next.
 */
Kirigami.Dialog {
    id: dialog

    // ContentInstallReview, already `ready`.
    property var review: null
    // The installer's key for it, handed back with the download.
    property string key: ""
    // Started once Install is pressed: the review's TaskWatcher.
    signal installStarted(var watcher, string key, string title)

    property bool started: false
    readonly property bool picked: !!dialog.review && dialog.review.primaryCount > 0

    property var queue: []
    function show(review, key) {
        if (dialog.visible) {
            dialog.queue = dialog.queue.concat([{ review: review, key: key }])
            return
        }
        dialog.review = review
        dialog.key = key || ""
        dialog.started = false
        dialog.open()
    }

    title: !dialog.review ? ""
         : dialog.picked ? i18nc("@title:dialog %1 is a mod's name", "Install %1", dialog.review.title)
         : dialog.review.title
    preferredWidth: Kirigami.Units.gridUnit * 28
    maximumHeight: applicationWindow().height - Kirigami.Units.gridUnit * 4
    padding: 0
    standardButtons: Kirigami.Dialog.Cancel

    customFooterActions: [
        Kirigami.Action {
            text: !dialog.review || dialog.review.checkedCount <= 1 ? i18nc("@action:button", "Install")
                : i18ncp("@action:button", "Install %1 File", "Install %1 Files", dialog.review.checkedCount)
            icon.name: "download"
            enabled: !!dialog.review && dialog.review.ready && dialog.review.checkedCount > 0
            onTriggered: {
                var review = dialog.review
                var title = review.title
                var watcher = review.install()
                dialog.started = !!watcher
                dialog.close()
                if (watcher)
                    dialog.installStarted(watcher, dialog.key, title)
            }
        }
    ]

    // Closing it any other way ends the review, so its resolver does not
    // linger; then the next one waiting, if any.
    onClosed: {
        if (dialog.review && !dialog.started)
            dialog.review.cancel()
        dialog.review = null
        if (dialog.queue.length > 0) {
            var next = dialog.queue[0]
            dialog.queue = dialog.queue.slice(1)
            Qt.callLater(dialog.show, next.review, next.key)
        }
    }

    ColumnLayout {
        spacing: 0

        QQC2.Label {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            text: !dialog.review ? ""
                  : !dialog.picked ? i18n("Mods in this instance need these, and they are not installed.")
                  : dialog.review.dependencyCount > 0 ? i18n("It needs these to run. Untick any you do not want.")
                  : i18n("Nothing else needs to come with it.")
            wrapMode: Text.Wrap
        }

        QQC2.CheckBox {
            Layout.leftMargin: Kirigami.Units.largeSpacing
            visible: !!dialog.review && dialog.review.dependencyCount > 1
            text: i18nc("@option:check", "All dependencies")
            tristate: true
            checkState: !dialog.review || dialog.review.checkedDependencyCount === 0 ? Qt.Unchecked
                      : dialog.review.checkedDependencyCount === dialog.review.dependencyCount ? Qt.Checked
                      : Qt.PartiallyChecked
            nextCheckState: () => dialog.review && dialog.review.checkedDependencyCount === dialog.review.dependencyCount
                                  ? Qt.Unchecked : Qt.Checked
            onClicked: dialog.review.setAllDependenciesChecked(checkState === Qt.Checked)
        }

        // The picked project first, as what is being installed; then each
        // dependency with its box.
        Repeater {
            model: dialog.review
            delegate: DelegateChooser {
                role: "isDependency"
                DelegateChoice {
                    roleValue: false
                    KD.SubtitleDelegate {
                        required property string name
                        required property string fileName
                        required property string provider
                        Layout.fillWidth: true
                        text: name
                        subtitle: [fileName].concat(provider.length > 0 ? [provider] : []).join(" · ")
                        icon.name: "download"
                        down: false
                        hoverEnabled: false
                    }
                }
                DelegateChoice {
                    roleValue: true
                    KD.CheckSubtitleDelegate {
                        id: entry
                        required property int index
                        required property string name
                        required property string fileName
                        required property string provider
                        required property string requiredBy
                        required property bool maybeInstalled
                        required property bool browserOnly
                        required property string versionType
                        required property var model

                        Layout.fillWidth: true
                        text: entry.name
                        subtitle: {
                            var parts = []
                            if (entry.maybeInstalled)
                                parts.push(i18n("Another version is already installed"))
                            else if (entry.browserOnly)
                                parts.push(i18n("The author only allows downloads from their own page"))
                            else if (entry.requiredBy.length > 0)
                                parts.push(i18nc("@info %1 is a list of mods", "Needed by %1", entry.requiredBy))
                            else
                                parts.push(entry.fileName)
                            if (entry.versionType === "beta")
                                parts.push(i18nc("@info a test release", "Beta"))
                            else if (entry.versionType === "alpha")
                                parts.push(i18nc("@info an early test release", "Alpha"))
                            if (entry.provider.length > 0)
                                parts.push(entry.provider)
                            return parts.join(" · ")
                        }
                        checked: entry.model.checked
                        onToggled: {
                            dialog.review.setChecked(entry.index, checked)
                            checked = Qt.binding(() => entry.model.checked)
                        }
                    }
                }
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            visible: !!dialog.review && dialog.review.unresolved.length > 0
            type: Kirigami.MessageType.Warning
            text: dialog.review
                  ? i18n("No compatible version was found for %1. It may not work without it.",
                         dialog.review.unresolved.join(", "))
                  : ""
        }
    }
}
