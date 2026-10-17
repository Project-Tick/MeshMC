// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MeshMC.Theme

/*
 * The "Java & memory" section of SettingsPage.qml: how much memory
 * Minecraft gets and which Java runtime runs it, launcher-wide. Each
 * instance can override these in its own InstanceJavaSettings.qml.
 *
 * The Java rows (path with Choose, Browse and Test, JVM arguments, the
 * download vendor, the Java MeshMC downloaded) work through JavaStore; see
 * JavaPathRow.qml and friends. PermGen only appears for a Java old enough
 * to use it.
 */
SettingsScroll {
    id: root

    // Installed memory in MiB: the ceiling for the memory slider.
    property int systemMemoryMiB: 8192

    readonly property var java: JavaStore.controller
    readonly property bool canDownload: !!java && java.downloaderAvailable
    property string removeError

    title: i18n("Java & memory")
    description: i18n("How much memory Minecraft gets, and which Java runtime runs it.")

    Connections {
        target: JavaStore.download
        ignoreUnknownSignals: true
        function onInstalled() {
            if (root.java)
                root.java.refreshManaged()
        }
    }

    SettingsGroup {
        width: parent.width
        title: i18n("Memory")
        MemorySetting {
            label: i18n("Maximum memory")
            systemMiB: root.systemMemoryMiB
        }
        MemoryStartSetting {
            label: i18n("Starting memory")
            description: i18n("Memory reserved when the game starts. Never above the maximum.")
        }
        SettingNumber {
            visible: javaPath.legacy
            key: "PermGen"
            label: i18n("PermGen size")
            description: i18n("Extra memory for Java older than 8. Newer Java ignores it.")
            from: 64
            to: 4096
            stepSize: 64
            suffix: i18n("MiB")
        }
    }

    SettingsGroup {
        width: parent.width
        title: i18n("Java runtime")
        JavaPathRow {
            id: javaPath
            label: i18n("Java executable")
            description: i18n("Used when an instance does not choose its own. Leave empty to let MeshMC pick.")
        }
        JvmArgsRow {
            label: i18n("JVM arguments")
            description: i18n("Extra flags for every launch. MeshMC already sets memory and the classpath.")
        }
    }

    SettingsGroup {
        width: parent.width
        title: i18n("Automatic Java")
        SettingSwitch {
            key: "JavaAutoDownload"
            label: i18n("Download Java automatically")
            description: i18n("Fetches the Java version each Minecraft release needs, so you never have to install one yourself.")
        }
        JavaVendorRow {
            enabled: SettingsStore.bool("JavaAutoDownload")
            label: i18n("Download from")
            description: i18n("Which vendor the automatic downloads come from.")
        }
    }

    Column {
        width: parent.width
        spacing: Theme.space.sm
        visible: root.canDownload

        Text {
            text: i18n("Downloaded Java")
            color: Theme.palette.textPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.type.title.pixelSize
            font.weight: Font.Bold
        }
        Text {
            width: parent.width
            text: i18n("Java that MeshMC downloaded and keeps in its own folder.")
            wrapMode: Text.Wrap
            color: Theme.palette.textTertiary
            font.family: Theme.font.family
            font.pixelSize: Theme.type.label.pixelSize
        }

        ManagedJavaList {
            width: parent.width
            currentPath: SettingsStore.string("JavaPath")
            onUseRequested: (path) => {
                root.removeError = ""
                SettingsStore.setValue("JavaPath", path)
            }
            onFailed: (message) => root.removeError = message
        }

        Text {
            width: parent.width
            visible: root.removeError.length > 0
            text: root.removeError
            wrapMode: Text.Wrap
            color: Theme.palette.danger
            font.family: Theme.font.family
            font.pixelSize: Theme.type.label.pixelSize
        }

        Row {
            spacing: Theme.space.sm
            Button {
                text: i18n("Download Java…")
                icon.source: Icons.url("download")
                onClicked: downloadDialog.open()
            }
            Button {
                flat: true
                text: i18n("Refresh")
                icon.source: Icons.url("refresh")
                onClicked: root.java.refreshManaged()
            }
        }
    }

    JavaDownloadDialog {
        id: downloadDialog
        onUseRequested: (path) => SettingsStore.setValue("JavaPath", path)
    }
}
