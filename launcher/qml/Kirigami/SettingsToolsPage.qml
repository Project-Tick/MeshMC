// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * External Tools: the profilers a launch can start under, MCEdit, and the
 * editor JSON and text files open in. Each path is typed or browsed for;
 * the tools get a Check that says whether they are really there.
 */
SettingsPage {
    id: page

    readonly property bool hasMCEdit: page.store.string("MCEditPath").length > 0

    FormCard.FormHeader {
        title: i18nc("@title:group", "Profilers")
    }
    FormCard.FormCard {
        SettingPath {
            source: page.store
            key: "JProfilerPath"
            label: i18nc("@label:textbox", "JProfiler install folder")
            folder: true
            toolId: "jprofiler"
        }
        FormCard.FormDelegateSeparator {}
        SettingPath {
            source: page.store
            key: "JVisualVMPath"
            label: i18nc("@label:textbox", "JVisualVM program")
            toolId: "jvisualvm"
        }
        FormCard.FormDelegateSeparator {}
        FormCard.FormLinkDelegate {
            text: i18nc("@action:button", "Get JProfiler")
            url: "https://www.ej-technologies.com/products/jprofiler/overview.html"
        }
        FormCard.FormLinkDelegate {
            text: i18nc("@action:button", "Get VisualVM")
            url: "https://visualvm.github.io/"
        }
    }

    // MCEdit has not been updated since 2018: offered only to whoever had
    // already set it up.
    FormCard.FormHeader {
        visible: page.hasMCEdit
        title: i18nc("@title:group", "MCEdit")
    }
    FormCard.FormCard {
        visible: page.hasMCEdit
        SettingPath {
            source: page.store
            key: "MCEditPath"
            label: i18nc("@label:textbox", "MCEdit install folder")
            folder: true
            toolId: "mcedit"
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Editor")
    }
    FormCard.FormCard {
        SettingPath {
            source: page.store
            key: "JsonEditor"
            label: i18nc("@label:textbox", "Text editor")
            placeholderText: i18nc("@info:placeholder", "This computer's default")
            description: i18n("Opens JSON and text files. Leave it empty to use the system's default.")
        }
    }
}
