// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * Java & Memory: how much memory Minecraft gets and which Java runs it,
 * launcher-wide (each instance can override both on its Settings tab),
 * automatic Java downloads, and the Java MeshMC downloaded. PermGen only
 * shows for a Java old enough to use it.
 */
SettingsPage {
    id: page

    readonly property var java: page.shell && page.shell.javaController ? page.shell.javaController : null

    FormCard.FormHeader {
        title: i18nc("@title:group", "Memory")
    }
    FormCard.FormCard {
        MaxMemoryDelegate {
            source: page.store
            systemMiB: page.shell && page.shell.systemMemoryMiB ? page.shell.systemMemoryMiB : 8192
        }
        FormCard.FormDelegateSeparator {}
        StartMemoryDelegate {
            source: page.store
        }
        FormCard.FormDelegateSeparator { visible: javaPath.legacy }
        SettingNumber {
            visible: javaPath.legacy
            source: page.store
            key: "PermGen"
            label: i18nc("@label:spinbox", "PermGen size")
            description: i18n("Extra memory for Java older than 8. Newer Java ignores it.")
            from: 64
            to: 4096
            stepSize: 64
            suffix: i18nc("@item:spinbox unit", "MiB")
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Java Runtime")
    }
    FormCard.FormCard {
        JavaPathDelegate {
            id: javaPath
            source: page.store
            java: page.java
            description: i18n("Used when an instance does not choose its own. Leave it empty to let MeshMC pick.")
        }
        FormCard.FormDelegateSeparator {}
        JvmArgsDelegate {
            source: page.store
            java: page.java
            description: i18n("Extra options for every launch. MeshMC already sets the memory and the class path.")
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Automatic Java")
    }
    FormCard.FormCard {
        SettingSwitch {
            id: autoDownload
            source: page.store
            key: "JavaAutoDownload"
            text: i18nc("@option:check", "Download Java automatically")
            description: i18n("Fetches the Java each Minecraft release needs, so you never have to install one yourself.")
        }
        FormCard.FormDelegateSeparator {}
        JavaVendorDelegate {
            source: page.store
            java: page.java
            enabled: autoDownload.checked
            description: i18n("Which vendor automatic downloads come from.")
        }
    }

    ManagedJavaCard {
        source: page.store
        java: page.java
        download: page.shell && page.shell.javaDownload ? page.shell.javaDownload : null
        onNotify: (message) => page.notify(message)
    }
}
