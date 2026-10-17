// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * Launcher Logs: MeshMC's own log files (a game's are on its instance's Log
 * tab). Pick a run's log -- MeshMC-0 is this one -- find in it, copy it,
 * upload it for a bug report (the link lands on the clipboard), or delete
 * old ones. The files are shell.launcherLogs (LauncherLogs: an
 * OtherLogsModel and its uploader).
 */
Kirigami.Page {
    id: page

    required property var app
    readonly property var launcherLogs: page.app && page.app.shell && page.app.shell.launcherLogs ? page.app.shell.launcherLogs : null
    readonly property var logs: page.launcherLogs ? page.launcherLogs.logs : null
    readonly property var uploader: page.launcherLogs ? page.launcherLogs.uploader : null

    readonly property bool hasFile: !!page.logs && page.logs.currentFile.length > 0
    readonly property var lines: page.hasFile ? page.logs.content.split("\n") : []
    property bool wrap: true
    property bool uploading: false
    property var matches: []
    property int current: -1
    readonly property int currentRow: page.current >= 0 ? page.matches[page.current] : -1
    readonly property var matchSet: {
        var set = {}
        for (var i = 0; i < page.matches.length; ++i)
            set[page.matches[i]] = true
        return set
    }

    // The files, newest first, in the model's order.
    property var files: []
    Instantiator {
        id: fileCollector
        model: page.logs
        delegate: QtObject {
            required property string name
        }
        onObjectAdded: Qt.callLater(page.collectFiles)
        onObjectRemoved: Qt.callLater(page.collectFiles)
    }
    function collectFiles() {
        var names = []
        for (var i = 0; i < fileCollector.count; ++i) {
            if (fileCollector.objectAt(i))
                names.push(fileCollector.objectAt(i).name)
        }
        page.files = names
        if (page.logs && page.logs.currentFile.length === 0 && names.length > 0)
            page.logs.selectFile(names[0])
    }

    function notify(message, actionText, callback) {
        var window = page.QQC2.ApplicationWindow.window
        if (window && typeof window.showPassiveNotification === "function")
            window.showPassiveNotification(message, "short", actionText || "", callback || null)
    }

    function findMatches() {
        var needle = searchField.text.toLowerCase()
        var found = []
        for (var i = 0; needle.length > 0 && i < page.lines.length; ++i) {
            if (page.lines[i].toLowerCase().indexOf(needle) >= 0)
                found.push(i)
        }
        page.matches = found
        page.current = found.length > 0 ? 0 : -1
    }
    function step(delta) {
        if (page.matches.length > 0)
            page.current = (page.current + delta + page.matches.length) % page.matches.length
    }
    onCurrentRowChanged: if (page.currentRow >= 0) view.positionAt(page.currentRow)
    onLinesChanged: findDelay.restart()

    title: i18nc("@title", "Launcher Logs")
    padding: 0

    Clipboard {
        id: clipboard
    }

    Connections {
        target: page.uploader
        ignoreUnknownSignals: true
        function onUploaded(url) {
            if (!page.uploading)
                return
            page.uploading = false
            clipboard.copyText(url)
            page.notify(i18n("Link copied to the clipboard."), i18nc("@action:button", "Open"),
                        () => Qt.openUrlExternally(url))
        }
        function onFailed(message) {
            if (!page.uploading)
                return
            page.uploading = false
            page.notify(message)
        }
    }

    LauncherLogsActions {
        id: logActions
        page: page
        clipboard: clipboard
    }
    actions: logActions.list

    header: QQC2.ToolBar {
        contentItem: RowLayout {
            spacing: Kirigami.Units.smallSpacing
            QQC2.ComboBox {
                model: page.files
                currentIndex: page.logs ? page.files.indexOf(page.logs.currentFile) : -1
                displayText: currentIndex < 0 ? i18nc("@item:inlistbox", "Choose a log file") : currentText
                onActivated: (index) => page.logs.selectFile(page.files[index])
                Accessible.name: i18nc("@label:listbox", "Log file")
            }
            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: i18nc("@info:placeholder", "Find in log…")
                enabled: page.hasFile
                onTextChanged: findDelay.restart()
                onAccepted: page.step(1)
                Keys.onUpPressed: page.step(-1)
                Keys.onDownPressed: page.step(1)
            }
            QQC2.Label {
                visible: searchField.text.length > 0
                text: page.matches.length > 0
                      ? i18nc("@info %1 of %2 matches", "%1 of %2", page.current + 1, page.matches.length)
                      : i18nc("@info", "No matches")
            }
        }
    }

    Timer {
        id: findDelay
        interval: Kirigami.Units.shortDuration
        onTriggered: page.findMatches()
    }

    LogView {
        id: view
        anchors.fill: parent
        model: page.lines
        wrap: page.wrap
        matchSet: page.matchSet
        currentRow: page.currentRow
        emptyText: !page.logs ? i18n("No launcher logs")
                 : page.files.length === 0 ? i18n("No logs yet")
                 : !page.hasFile ? i18n("No file chosen") : ""
        emptyExplanation: page.files.length > 0 && !page.hasFile ? i18n("Choose a log file above to read it.") : ""
    }
}
