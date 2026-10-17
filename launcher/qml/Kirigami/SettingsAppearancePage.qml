// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * Appearance: the launcher's language, and away from Plasma its colour
 * scheme (shell.colorSchemes; on Plasma the desktop's own applies, and the
 * picker is left out). Both apply at once.
 */
SettingsPage {
    id: page

    readonly property var languages: page.shell && page.shell.languages ? page.shell.languages : null
    readonly property var schemes: page.shell && page.shell.colorSchemes ? page.shell.colorSchemes : null
    readonly property bool schemesAvailable: !!page.schemes && page.schemes.available && !!page.schemes.model

    // KColorSchemeModel names only its display and decoration roles; the
    // scheme id is the role after Qt.UserRole (KColorSchemeModel::IdRole).
    readonly property int schemeIdRole: Qt.UserRole + 1
    function schemeId(row) {
        var model = page.schemes.model
        return String(model.data(model.index(row, 0), page.schemeIdRole) || "")
    }
    function schemeRow(id) {
        var model = page.schemes.model
        for (var row = 0; row < model.rowCount(); ++row) {
            if (page.schemeId(row) === id)
                return row
        }
        return 0
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Language")
    }
    FormCard.FormCard {
        FormCard.FormComboBoxDelegate {
            id: languageBox
            text: i18nc("@label:listbox", "Display language")
            description: i18n("Applied right away.")
            model: page.languages
            textRole: "name"
            valueRole: "languageKey"
            enabled: !!page.languages && !!page.shell && typeof page.shell.selectLanguage === "function"
            readonly property string stored: page.store.string("Language")
            function sync() { currentIndex = indexOfValue(languageBox.stored) }
            onStoredChanged: sync()
            onCountChanged: sync()
            Component.onCompleted: sync()
            onActivated: {
                page.shell.selectLanguage(currentValue)
                sync()
            }
        }
    }

    FormCard.FormHeader {
        visible: page.schemesAvailable
        title: i18nc("@title:group", "Colors")
    }
    FormCard.FormCard {
        visible: page.schemesAvailable
        FormCard.FormComboBoxDelegate {
            id: schemeBox
            text: i18nc("@label:listbox", "Color scheme")
            description: i18n("“Default” follows this computer's light or dark appearance.")
            model: page.schemesAvailable ? page.schemes.model : null
            textRole: "display"
            readonly property string chosen: page.schemesAvailable ? page.schemes.chosen : ""
            function sync() { currentIndex = page.schemesAvailable && count > 0 ? page.schemeRow(schemeBox.chosen) : -1 }
            onChosenChanged: sync()
            onCountChanged: sync()
            Component.onCompleted: sync()
            onActivated: (index) => {
                page.schemes.choose(page.schemeId(index))
                sync()
            }
        }
    }
}
