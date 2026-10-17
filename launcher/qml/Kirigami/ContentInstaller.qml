// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * Every install Add Content starts for one instance, kept on the instance's
 * page so they outlive the Add Content page that started them.
 *
 * An install is: pick a version (the newest one that runs on this instance,
 * unless the project page picked another), ask the browser for a review --
 * ContentInstallReview works out the dependencies -- and then either
 * download at once, when there is nothing to ask about, or hand the review
 * to `reviewRequested` for InstallReviewDialog. Installs run side by side,
 * each with its own TaskWatcher, so browsing on never waits for a download.
 *
 * The browser has one version list at a time, for the result it was last
 * asked about, so version requests go one after another (requestVersions()).
 * Result rows are positions in the browser's current results; a new search,
 * kind or site moves them, and a request made before that is dropped rather
 * than applied to whatever is in that row now (`generation`).
 *
 * The folder re-reads itself once a download ends (ContentInstallReview),
 * which is what puts the new files on the Content tab.
 */
QtObject {
    id: installer

    required property var app
    property var details: null
    // Set by the first Add Content page: until then the browser is not
    // made, so an instance page never talks to the network on its own.
    property bool active: false
    readonly property var browser: installer.active && installer.details ? (installer.details.contentBrowser || null) : null

    // A review is ready and needs the user (InstallReviewDialog).
    signal reviewRequested(var review, string key)

    // Downloads and reviews in progress, by project key. Plain objects do
    // not notify, so `revision` is bumped on every change and read by
    // whatever looks one up.
    property var downloads: ({})
    property var checking: ({})
    property int revision: 0
    readonly property int activeCount: {
        installer.revision
        var n = 0
        for (var key in installer.downloads) {
            if (installer.downloads[key] && installer.downloads[key].running)
                ++n
        }
        for (var other in installer.checking)
            ++n
        return n
    }
    // The running downloads' progress together, or -1 while any cannot say.
    readonly property real activeProgress: {
        installer.revision
        var sum = 0
        var n = 0
        for (var key in installer.downloads) {
            var d = installer.downloads[key]
            if (!d || !d.running)
                continue
            if (d.progress < 0)
                return -1
            sum += d.progress
            ++n
        }
        return n > 0 && Object.keys(installer.checking).length === 0 ? sum / n : -1
    }

    // Bumped whenever the result rows move (a search, another kind or site).
    property int generation: 0
    // Every request still waiting is answered with null: its row is gone.
    function resultsMoved() {
        installer.generation++
        var dropped = installer.versionQueue
        installer.versionQueue = []
        if (installer.versionRequest) {
            dropped.unshift(installer.versionRequest)
            installer.versionRequest = null
        }
        for (var i = 0; i < dropped.length; ++i)
            dropped[i].callback(null)
    }

    function projectKey(title, author) {
        var b = installer.browser
        return (b ? b.provider + "/" + b.contentType : "") + "/" + title + "/" + author
    }
    function downloadFor(key) {
        installer.revision
        return installer.downloads[key] || null
    }
    function isChecking(key) {
        installer.revision
        return !!installer.checking[key]
    }
    function isBusy(key) {
        var d = installer.downloadFor(key)
        return installer.isChecking(key) || (!!d && d.running)
    }

    // Why installing is off right now, if it is.
    readonly property string blockedReason: !installer.browser ? ""
        : !installer.browser.changesAllowed ? i18n("The game is running. Mods can be installed once it closes.")
        : !installer.browser.installTargetAvailable ? i18n("This instance's shared data pack folder is turned off.")
        : ""

    /* Version requests, one at a time: {row, generation, callback}. The
     * callback gets the versions of that row (ContentBrowser.versions), or
     * null when the row moved before its turn. */
    property var versionQueue: []
    property var versionRequest: null
    function requestVersions(row, callback) {
        installer.versionQueue = installer.versionQueue.concat([{
            row: row, generation: installer.generation, callback: callback
        }])
        installer.nextVersions()
    }
    function nextVersions() {
        if (installer.versionRequest || installer.versionQueue.length === 0 || !installer.browser)
            return
        var request = installer.versionQueue[0]
        installer.versionQueue = installer.versionQueue.slice(1)
        if (request.generation !== installer.generation) {
            request.callback(null)
            Qt.callLater(installer.nextVersions)
            return
        }
        installer.versionRequest = request
        installer.browser.loadVersions(request.row)
        installer.settleVersions()
    }
    function settleVersions() {
        var request = installer.versionRequest
        if (!request || !installer.browser || installer.browser.versionsLoading)
            return
        installer.versionRequest = null
        request.callback(request.generation === installer.generation ? installer.browser.versions : null)
        Qt.callLater(installer.nextVersions)
    }
    readonly property Connections versionsWatch: Connections {
        target: installer.browser
        function onVersionsLoadingChanged() {
            installer.settleVersions()
        }
        function onResultsChanged() {
            installer.resultsMoved()
        }
        // A new search starts from no rows; more of the same search only
        // adds rows after the ones there are.
        function onCountChanged() {
            var count = installer.browser.count
            if (count < installer.lastCount)
                installer.resultsMoved()
            installer.lastCount = count
        }
    }
    property int lastCount: 0

    // The newest version that runs on this instance, else the newest.
    function bestVersion(versions) {
        if (!versions || versions.length === 0)
            return null
        return versions.find(v => v.isCompatible) || versions[0]
    }

    // Installs @p project (row @p row's {title, author, ...}) at its best
    // version, from a result row's own button.
    function quickInstall(row, project) {
        var key = installer.projectKey(project.title, project.author)
        if (installer.isBusy(key) || installer.blockedReason.length > 0)
            return
        installer.checking[key] = true
        installer.revision++
        installer.requestVersions(row, function (versions) {
            var version = installer.bestVersion(versions)
            if (!version) {
                delete installer.checking[key]
                installer.revision++
                installer.app.notify(versions ? i18n("“%1” has no version to install.", project.title)
                                              : i18n("The results changed before “%1” could be installed.", project.title))
                return
            }
            installer.install(row, project, version.id)
        })
    }

    // Installs version @p versionId of @p project, row @p row: its review
    // first, then the download. False when the browser refuses it.
    function install(row, project, versionId) {
        var key = installer.projectKey(project.title, project.author)
        var review = installer.browser ? installer.browser.review(row, versionId) : null
        if (!review) {
            delete installer.checking[key]
            installer.revision++
            installer.app.notify(i18n("This version of “%1” cannot be installed here.", project.title))
            return false
        }
        installer.checking[key] = review
        installer.revision++
        var settle = function () {
            if (!review.ready || installer.checking[key] !== review)
                return
            delete installer.checking[key]
            installer.revision++
            if (review.needsReview)
                installer.reviewRequested(review, key)
            else
                installer.track(key, review.install(), project.title)
        }
        review.stateChanged.connect(settle)
        settle()
        return true
    }

    // Follows the download @p watcher of @p key, and says how it went.
    function track(key, watcher, title) {
        if (!watcher)
            return
        installer.downloads[key] = watcher
        installer.revision++
        watcher.runningChanged.connect(() => installer.revision++)
        watcher.finished.connect(function (ok) {
            installer.revision++
            installer.app.notify(ok ? i18n("Installed “%1”.", title)
                                    : i18n("Could not install “%1”: %2", title, watcher.error))
        })
    }
}
