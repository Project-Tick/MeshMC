// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * The Content tab's update flows, on top of ContentUpdateController:
 * Check for Updates (UpdatesDialog) and Verify Dependencies (what the
 * installed mods need and lack, through InstallReviewDialog). Whatever
 * downloads is `activity` until it ends, for the tab's footer; how it went
 * is said with a passive notification. The folder re-reads itself once a
 * download ends, which is what updates the list.
 */
Item {
    id: flows

    required property var app
    // InstanceContentController, and the kind the tab is showing.
    property var content: null
    property string kind: "mods"

    readonly property var updater: flows.content ? flows.content.updater : null
    // The download in progress (TaskWatcher), or null.
    property var activity: null
    // A review still working out what it needs, or null.
    property var resolving: null
    readonly property bool busy: !!flows.activity || !!flows.resolving
    readonly property string busyText: flows.activity
                                       ? (flows.activity.status.length > 0 ? flows.activity.status : flows.activity.title)
                                       : flows.resolving ? i18nc("@info:status", "Checking what they need…") : ""
    readonly property real busyProgress: flows.activity ? flows.activity.progress : -1

    function supportsUpdates() {
        return !!flows.updater && flows.updater.supportsUpdates(flows.kind)
    }

    function checkUpdates() {
        if (!flows.updater)
            return false
        var watcher = flows.updater.checkForUpdates(flows.kind, [])
        if (!watcher) {
            flows.app.notify(flows.updater.lastError)
            return false
        }
        updatesDialog.show(watcher)
        return true
    }

    function verifyDependencies() {
        if (!flows.updater)
            return
        var review = flows.updater.verifyDependencies()
        if (!review) {
            flows.app.notify(flows.updater.lastError)
            return
        }
        flows.resolving = review
        var settle = function () {
            if (!review.ready || flows.resolving !== review)
                return
            flows.resolving = null
            if (review.needsReview) {
                reviewDialog.show(review, "")
            } else {
                review.cancel()
                flows.app.notify(i18n("Every dependency is installed."))
            }
        }
        review.stateChanged.connect(settle)
        settle()
    }

    function track(watcher, doneText) {
        if (!watcher)
            return
        flows.activity = watcher
        watcher.finished.connect(function (ok) {
            flows.app.notify(ok ? doneText
                                : (watcher.error.length > 0 ? watcher.error : i18n("The download did not finish.")))
            if (flows.activity === watcher)
                flows.activity = null
        })
    }

    UpdatesDialog {
        id: updatesDialog
        controller: flows.updater
        onUpdateStarted: (watcher) => flows.track(watcher, i18n("Updates installed."))
    }

    InstallReviewDialog {
        id: reviewDialog
        onInstallStarted: (watcher, key, title) => flows.track(watcher, i18n("Installed the missing dependencies."))
    }
}
