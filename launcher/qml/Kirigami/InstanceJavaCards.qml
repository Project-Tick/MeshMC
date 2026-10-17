// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * The Java part of one instance's settings: memory, which Java runs it
 * (Choose, Browse and Test), its JVM arguments and where automatic Java
 * comes from -- each following Settings, Java & Memory, until the instance
 * has its own. Downloading and removing Java is launcher-wide, so a button
 * leads there.
 */
ColumnLayout {
    id: cards

    required property var app
    required property var source
    readonly property var shell: cards.app.shell
    readonly property var java: cards.shell && cards.shell.javaController ? cards.shell.javaController : null

    Layout.fillWidth: true
    spacing: 0

    FormCard.FormHeader {
        title: i18nc("@title:group", "Memory")
    }
    OverridableCard {
        id: memory
        source: cards.source
        overrideKey: "OverrideMemory"
        overrideKeys: ["MinMemAlloc", "MaxMemAlloc", "PermGen"]
        MaxMemoryDelegate {
            source: cards.source
            enabled: memory.editable
            systemMiB: cards.shell && cards.shell.systemMemoryMiB ? cards.shell.systemMemoryMiB : 8192
        }
        FormCard.FormDelegateSeparator {}
        StartMemoryDelegate {
            source: cards.source
            enabled: memory.editable
        }
        FormCard.FormDelegateSeparator { visible: javaPath.legacy }
        SettingNumber {
            visible: javaPath.legacy
            source: cards.source
            key: "PermGen"
            enabled: memory.editable
            label: i18nc("@label:spinbox", "PermGen size")
            description: i18n("Extra memory for Java older than 8. Newer Java ignores it.")
            from: 64
            to: 4096
            stepSize: 64
            suffix: i18nc("@item:spinbox unit", "MiB")
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Java")
    }
    OverridableCard {
        id: location
        source: cards.source
        overrideKey: "OverrideJavaLocation"
        overrideKeys: ["JavaPath"]
        overrideText: i18nc("@option:check", "Use a specific Java for this instance")
        JavaPathDelegate {
            id: javaPath
            source: cards.source
            java: cards.java
            enabled: location.editable
        }
        FormCard.FormDelegateSeparator {}
        FormCard.FormButtonDelegate {
            visible: !!cards.java
            text: i18nc("@action:button", "Download or Remove Java in Settings")
            icon.name: "configure"
            onClicked: cards.app.navigate("settings/java")
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "JVM Arguments")
    }
    OverridableCard {
        id: args
        source: cards.source
        overrideKey: "OverrideJavaArgs"
        overrideKeys: ["JvmArgs"]
        overrideText: i18nc("@option:check", "Use custom JVM arguments for this instance")
        JvmArgsDelegate {
            source: cards.source
            java: cards.java
            enabled: args.editable
        }
    }

    FormCard.FormHeader {
        visible: vendorCard.visible
        title: i18nc("@title:group", "Automatic Java")
    }
    OverridableCard {
        id: vendorCard
        visible: !!cards.java && cards.java.downloaderAvailable
        source: cards.source
        overrideKey: "OverrideJavaAutoDownloadVendor"
        overrideKeys: ["JavaAutoDownloadVendor"]
        overrideText: i18nc("@option:check", "Download Java from a different vendor for this instance")
        JavaVendorDelegate {
            source: cards.source
            java: cards.java
            enabled: vendorCard.editable
        }
    }
}
