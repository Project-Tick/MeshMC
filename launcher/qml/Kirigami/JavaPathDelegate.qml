// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import org.kde.kirigami as Kirigami

/*
 * Which Java runs the game: the path, typed or picked -- "Choose…" lists the
 * installs MeshMC found, Browse is the system's file dialog -- and Test,
 * which runs that Java the way a launch does (with the JVM arguments and
 * memory of `source`) and says under the field what came back.
 *
 * For the launcher's setting and, under an override switch, one instance's
 * (`source` is then the instance's settings). Without `java` (the
 * JavaController) it is the plain field.
 */
SettingText {
    id: delegate

    // JavaController: installs, detect(), test(), pickedJava(), ...
    property var java: null
    property string argsKey: "JvmArgs"
    // Tells a test started here from one another view started.
    readonly property string token: "java-" + Math.random()
    readonly property bool testing: !!delegate.java && delegate.java.testingToken === delegate.token

    // Whether the chosen Java is old enough to take PermGen: from its last
    // pick or test, else from the version the launcher last saw run; -1
    // while neither has said.
    property int legacyHint: -1
    readonly property bool legacy: delegate.legacyHint >= 0
                                   ? delegate.legacyHint === 1
                                   : !!delegate.java && delegate.java.requiresPermGen(delegate.source.string("JavaVersion"))

    key: "JavaPath"
    label: i18nc("@label:textbox", "Java executable")
    placeholderText: i18nc("@info:placeholder", "Automatic")

    function setPath(path, hint) {
        delegate.text = path
        delegate.cursorPosition = 0
        delegate.source.setValue(delegate.key, path)
        delegate.statusMessage = ""
        delegate.legacyHint = hint
    }

    function runTest() {
        var path = delegate.text.trim()
        if (!delegate.java || path.length === 0)
            return
        delegate.status = Kirigami.MessageType.Information
        delegate.statusMessage = i18n("Checking this Java…")
        delegate.java.test(delegate.token, path, delegate.source.string(delegate.argsKey),
                           delegate.source.number("MinMemAlloc"), delegate.source.number("MaxMemAlloc"),
                           delegate.source.number("PermGen"))
    }

    onTextEdited: {
        delegate.statusMessage = ""
        delegate.legacyHint = -1
    }

    Connections {
        target: delegate.java
        ignoreUnknownSignals: true
        function onTestFinished(token, outcome) {
            if (token !== delegate.token)
                return
            delegate.status = outcome.ok ? (outcome.warnings.length > 0 ? Kirigami.MessageType.Warning
                                                                         : Kirigami.MessageType.Positive)
                                         : Kirigami.MessageType.Error
            if (!outcome.ok) {
                delegate.statusMessage = outcome.error
                return
            }
            var line = i18n("Works: Java %1, %2, from %3.", outcome.version, outcome.architecture, outcome.vendor)
            delegate.statusMessage = outcome.warnings.length > 0 ? line + "\n" + outcome.warnings : line
            delegate.legacyHint = outcome.requiresPermGen ? 1 : 0
        }
    }

    trailing: [
        QQC2.Button {
            visible: !!delegate.java
            text: i18nc("@action:button", "Choose…")
            onClicked: picker.open()
        },
        QQC2.Button {
            visible: !!delegate.java
            icon.name: "document-open-folder"
            text: i18nc("@action:button", "Browse…")
            display: QQC2.AbstractButton.IconOnly
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: browse.open()
        },
        QQC2.Button {
            visible: !!delegate.java
            text: delegate.testing ? i18nc("@action:button", "Testing…") : i18nc("@action:button", "Test")
            enabled: !delegate.testing && delegate.text.trim().length > 0 && !!delegate.java
                     && delegate.java.testingToken.length === 0
            onClicked: delegate.runTest()
        }
    ]

    JavaPickerDialog {
        id: picker
        java: delegate.java
        currentPath: delegate.text
        onPicked: (path, version) => delegate.setPath(path, delegate.java.requiresPermGen(version) ? 1 : 0)
    }

    Dialogs.FileDialog {
        id: browse
        title: i18nc("@title:window", "Find Java")
        onAccepted: {
            var picked = delegate.java ? delegate.java.pickedJava(selectedFile) : ""
            if (picked.length === 0) {
                delegate.status = Kirigami.MessageType.Error
                delegate.statusMessage = i18n("That file cannot be run as Java.")
                return
            }
            delegate.setPath(picked, -1)
            delegate.runTest()
        }
    }
}
