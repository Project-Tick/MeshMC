// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import MeshMC.Theme

/*
 * Profilers, MCEdit, and the editor used for JSON/text files -- the QML
 * equivalent of the classic ExternalToolsPage. Each path is picked from a
 * native dialog (see SettingPathField.qml) rather than typed by hand, and
 * JProfiler/JVisualVM/MCEdit each get a Check button next to their path,
 * same as the classic page.
 */
SettingsScroll {
    title: i18n("External tools")
    description: i18n("Profilers, MCEdit, and the editor used for JSON and text files.")

    SettingsGroup {
        width: parent.width
        title: i18n("JProfiler")
        description: i18n("https://www.ej-technologies.com/products/jprofiler/overview.html")
        SettingPathField {
            key: "JProfilerPath"
            label: i18n("Install folder")
            folder: true
            toolId: "jprofiler"
        }
    }

    SettingsGroup {
        width: parent.width
        title: i18n("JVisualVM")
        description: i18n("https://visualvm.github.io/")
        SettingPathField {
            key: "JVisualVMPath"
            label: i18n("Executable")
            toolId: "jvisualvm"
        }
    }

    // MCEdit has not been updated since 2018; the group stays for anyone who
    // already set a path, and is left out for everyone else.
    SettingsGroup {
        width: parent.width
        visible: SettingsStore.string("MCEditPath").length > 0
        title: i18n("MCEdit")
        description: i18n("https://www.mcedit.net/")
        SettingPathField {
            key: "MCEditPath"
            label: i18n("Install folder")
            folder: true
            toolId: "mcedit"
        }
    }

    SettingsGroup {
        width: parent.width
        title: i18n("Editors")
        description: i18n("Leave empty to use the system default.")
        SettingPathField {
            key: "JsonEditor"
            label: i18n("Text editor")
            placeholder: i18n("Automatic")
        }
    }
}
