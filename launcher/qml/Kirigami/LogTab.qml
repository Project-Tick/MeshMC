// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * The game's log: the live one while the game runs (InstanceLogBridge's
 * LogModel, coloured by level), or one of the instance's log files on disk
 * (OtherLogsModel). Find highlights the lines that contain the search text
 * and steps through them; Copy and Upload take the whole log, and Upload
 * leaves its link on the clipboard.
 */
Item {
    id: tab

    required property var app
    property var details: null
    // Whether this tab is the one on screen (it then answers Ctrl+F).
    property bool isCurrent: false

    readonly property var log: tab.details && tab.details.log ? tab.details.log : null
    readonly property var otherLogs: tab.details && tab.details.otherLogs ? tab.details.otherLogs : null
    readonly property var uploader: tab.details && tab.details.logUploader ? tab.details.logUploader : null

    // The file on show, or "" for the live log.
    property string file: ""
    readonly property bool live: tab.file.length === 0
    readonly property var fileLines: !tab.live && tab.otherLogs ? tab.otherLogs.content.split("\n") : []
    readonly property var shownModel: tab.live ? (tab.log ? tab.log.model : null) : tab.fileLines
    readonly property bool hasText: tab.live ? !!(tab.log && tab.log.model) : tab.fileLines.length > 0

    property bool wrap: true
    // Keep the newest line in view as lines arrive (the live log only).
    property bool follow: true
    // Rows that contain the search text, ascending, and the one looked at.
    property var matches: []
    property int current: -1
    property bool uploading: false

    // The log files, in the model's order.
    property var files: []
    function collectFiles() {
        var names = []
        for (var i = 0; i < fileCollector.count; ++i) {
            var entry = fileCollector.objectAt(i)
            if (entry)
                names.push(entry.name)
        }
        tab.files = names
    }
    Instantiator {
        id: fileCollector
        model: tab.otherLogs
        delegate: QtObject {
            required property string name
        }
        onObjectAdded: Qt.callLater(tab.collectFiles)
        onObjectRemoved: Qt.callLater(tab.collectFiles)
    }

    function showFile(name) {
        tab.file = name
        if (name.length > 0 && tab.otherLogs)
            tab.otherLogs.selectFile(name)
        tab.findMatches()
    }

    function text() {
        return tab.live ? (tab.log ? tab.log.text() : "") : tab.otherLogs.content
    }

    function findMatches() {
        var needle = searchField.text
        var found = []
        if (needle.length > 0 && tab.live && tab.log)
            found = tab.log.findLines(needle)
        else if (needle.length > 0) {
            var lower = needle.toLowerCase()
            for (var i = 0; i < tab.fileLines.length; ++i) {
                if (tab.fileLines[i].toLowerCase().indexOf(lower) >= 0)
                    found.push(i)
            }
        }
        tab.matches = found
        tab.current = found.length > 0 ? 0 : -1
    }
    function step(delta) {
        if (tab.matches.length === 0)
            return
        tab.current = (tab.current + delta + tab.matches.length) % tab.matches.length
    }
    readonly property int currentRow: tab.current >= 0 && tab.current < tab.matches.length ? tab.matches[tab.current] : -1
    onCurrentRowChanged: {
        if (tab.currentRow >= 0) {
            tab.follow = false
            lines.positionAt(tab.currentRow)
        }
    }
    readonly property var matchSet: {
        var set = {}
        for (var i = 0; i < tab.matches.length; ++i)
            set[tab.matches[i]] = true
        return set
    }

    Clipboard {
        id: clipboard
    }

    Connections {
        target: tab.uploader
        function onUploaded(url) {
            if (!tab.uploading)
                return
            tab.uploading = false
            clipboard.copyText(url)
            tab.app.notify(i18n("Link copied to the clipboard."), i18nc("@action:button", "Open"),
                           () => Qt.openUrlExternally(url))
        }
        function onFailed(message) {
            if (!tab.uploading)
                return
            tab.uploading = false
            tab.app.notify(message)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        QQC2.ToolBar {
            Layout.fillWidth: true
            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.ComboBox {
                    visible: tab.files.length > 0
                    model: [i18nc("@item:inlistbox", "Live Game Log")].concat(tab.files)
                    currentIndex: tab.live ? 0 : tab.files.indexOf(tab.file) + 1
                    onActivated: (index) => tab.showFile(index === 0 ? "" : tab.files[index - 1])
                    Accessible.name: i18nc("@label:listbox", "Log")
                }
                Kirigami.SearchField {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 16
                    placeholderText: i18nc("@info:placeholder", "Find in log…")
                    enabled: tab.hasText
                    focusSequences: tab.isCurrent ? [StandardKey.Find] : []
                    onTextChanged: findDelay.restart()
                    onAccepted: tab.step(1)
                    Keys.onUpPressed: tab.step(-1)
                    Keys.onDownPressed: tab.step(1)
                }
                QQC2.Label {
                    visible: searchField.text.length > 0
                    text: tab.matches.length > 0
                          ? i18nc("@info %1 of %2 matches", "%1 of %2", tab.current + 1, tab.matches.length)
                          : i18nc("@info", "No matches")
                }
                Kirigami.ActionToolBar {
                    Layout.fillWidth: true
                    alignment: Qt.AlignRight
                    actions: [
                        Kirigami.Action {
                            text: i18nc("@action:button", "Previous Match")
                            icon.name: "go-up"
                            displayHint: Kirigami.DisplayHint.IconOnly
                            enabled: tab.matches.length > 0
                            onTriggered: tab.step(-1)
                        },
                        Kirigami.Action {
                            text: i18nc("@action:button", "Next Match")
                            icon.name: "go-down"
                            displayHint: Kirigami.DisplayHint.IconOnly
                            enabled: tab.matches.length > 0
                            onTriggered: tab.step(1)
                        },
                        Kirigami.Action {
                            text: i18nc("@action:button", "Follow")
                            icon.name: "go-bottom"
                            checkable: true
                            checked: tab.follow
                            visible: tab.live
                            onToggled: {
                                tab.follow = checked
                            }
                        },
                        Kirigami.Action {
                            text: i18nc("@action:button", "Wrap Lines")
                            icon.name: "text-wrap"
                            checkable: true
                            checked: tab.wrap
                            onToggled: tab.wrap = checked
                        },
                        Kirigami.Action {
                            text: i18nc("@action:button", "Copy")
                            icon.name: "edit-copy"
                            enabled: tab.hasText
                            onTriggered: {
                                clipboard.copyText(tab.text())
                                tab.app.notify(i18n("Log copied."))
                            }
                        },
                        Kirigami.Action {
                            text: tab.uploading ? i18nc("@action:button", "Uploading…") : i18nc("@action:button", "Upload")
                            icon.name: "cloud-upload"
                            visible: !!tab.uploader
                            enabled: tab.hasText && !!tab.uploader && !tab.uploader.busy
                            onTriggered: {
                                tab.uploading = true
                                tab.uploader.uploadText(tab.text())
                            }
                        },
                        Kirigami.Action {
                            text: i18nc("@action:button", "Clear")
                            icon.name: "edit-clear-history"
                            visible: tab.live
                            enabled: tab.hasText
                            displayHint: Kirigami.DisplayHint.AlwaysHide
                            onTriggered: tab.log.clear()
                        },
                        Kirigami.Action {
                            text: i18nc("@action:button", "Open Logs Folder")
                            icon.name: "document-open-folder"
                            visible: !!tab.otherLogs
                            displayHint: Kirigami.DisplayHint.AlwaysHide
                            onTriggered: tab.app.call("openPath", tab.otherLogs.path)
                        }
                    ]
                }
            }
        }

        Timer {
            id: findDelay
            interval: Kirigami.Units.shortDuration
            onTriggered: tab.findMatches()
        }

        LogView {
            id: lines
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: tab.shownModel
            wrap: tab.wrap
            follow: tab.follow && tab.live
            matchSet: tab.matchSet
            currentRow: tab.currentRow
            emptyText: !tab.hasText ? (tab.live ? i18n("No game log yet") : i18n("This log file is empty")) : ""
            emptyExplanation: tab.live ? i18n("The log appears here while the game starts and runs.") : ""
            onScrolledAway: tab.follow = false
            onReachedEnd: if (tab.live) tab.follow = true
        }
    }
}
