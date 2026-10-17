// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import MeshMC.Theme

/*
 * The Java part of one instance's settings (InstanceSettingsTab.qml):
 * memory, which Java runs the game (with Choose, Browse and Test, see
 * JavaPathRow.qml), its JVM arguments and the vendor automatic downloads
 * come from. Each group follows the launcher-wide value (Settings, "Java &
 * memory") until switched to "custom for this instance". The tab hands in
 * its own SettingsSource and disables this whole column while the game
 * runs. Downloading and removing Java is launcher-wide, so a link points
 * to Settings for that.
 */
Column {
    id: root

    // The tab's SettingsSource over this instance's settings.
    property var source
    property int systemMemoryMiB: 8192

    spacing: Theme.space.xl

    OverrideGroup {
        id: memory
        width: parent.width
        title: i18n("Memory")
        source: root.source
        gateKey: "OverrideMemory"
        keys: ["MinMemAlloc", "MaxMemAlloc", "PermGen"]
        MemorySetting {
            source: root.source
            enabled: memory.overriding
            label: i18n("Maximum memory")
            systemMiB: root.systemMemoryMiB
        }
        MemoryStartSetting {
            source: root.source
            enabled: memory.overriding
            label: i18n("Starting memory")
        }
        SettingNumber {
            source: root.source
            key: "PermGen"
            visible: javaLocation.legacy
            enabled: memory.overriding
            label: i18n("PermGen size")
            description: i18n("Extra memory for Java older than 8. Newer Java ignores it.")
            from: 64
            to: 4096
            stepSize: 64
            suffix: i18n("MiB")
        }
    }

    // The Java choice and, under it, the way to the launcher-wide Java
    // downloads.
    Column {
        width: parent.width
        spacing: Theme.space.sm

        OverrideGroup {
            id: javaPath
            width: parent.width
            title: i18n("Java")
            source: root.source
            gateKey: "OverrideJavaLocation"
            gateLabel: i18n("Use a specific Java for this instance")
            keys: ["JavaPath"]
            JavaPathRow {
                id: javaLocation
                source: root.source
                enabled: javaPath.overriding
                label: i18n("Java executable")
            }
        }

        LinkButton {
            x: parent.width - width
            visible: !!JavaStore.controller
            text: i18n("Download or remove Java in Settings")
            onClicked: JavaStore.navigateRequested("settings/java")
        }
    }

    OverrideGroup {
        id: javaArgs
        width: parent.width
        title: i18n("JVM arguments")
        source: root.source
        gateKey: "OverrideJavaArgs"
        gateLabel: i18n("Use custom JVM arguments for this instance")
        keys: ["JvmArgs"]
        JvmArgsRow {
            source: root.source
            enabled: javaArgs.overriding
            label: i18n("Arguments")
        }
    }

    OverrideGroup {
        id: javaVendor
        width: parent.width
        visible: !!JavaStore.controller && JavaStore.controller.downloaderAvailable
        title: i18n("Automatic Java")
        source: root.source
        gateKey: "OverrideJavaAutoDownloadVendor"
        gateLabel: i18n("Download Java from a different vendor for this instance")
        keys: ["JavaAutoDownloadVendor"]
        JavaVendorRow {
            source: root.source
            enabled: javaVendor.overriding
            label: i18n("Download from")
        }
    }
}
