// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * Updates: whether MeshMC looks for new versions by itself, how often, and
 * a check now (UpdateSettingsController). Only offered in a build that can
 * update itself (`available`).
 */
SettingsPage {
    id: page

    readonly property var updates: page.shell && page.shell.updateSettings ? page.shell.updateSettings : null

    FormCard.FormHeader {
        title: i18nc("@title:group", "Update Checks")
    }
    FormCard.FormCard {
        FormCard.FormSwitchDelegate {
            id: autoCheck
            text: i18nc("@option:check", "Check for updates automatically")
            description: i18n("MeshMC asks in the background and tells you when a new version is out.")
            checked: !!page.updates && page.updates.autoCheck
            onToggled: {
                page.updates.autoCheck = checked
                checked = Qt.binding(() => !!page.updates && page.updates.autoCheck)
            }
        }
        FormCard.FormDelegateSeparator {}
        SpinBoxDelegate {
            label: i18nc("@label:spinbox", "How often")
            description: i18n("0 checks only when MeshMC starts.")
            enabled: autoCheck.checked
            from: 0
            to: 24 * 30
            stepSize: 6
            value: page.updates ? page.updates.intervalHours : 0
            textFromValue: (value, locale) => value === 0 ? i18nc("@item:spinbox", "At start")
                                                          : i18ncp("@item:spinbox", "Every hour", "Every %1 hours", value)
            valueFromText: (text, locale) => {
                var digits = text.replace(/[^0-9]/g, "")
                return digits.length > 0 ? parseInt(digits, 10) : 0
            }
            onValueModified: {
                page.updates.intervalHours = value
                value = Qt.binding(() => page.updates ? page.updates.intervalHours : 0)
            }
        }
        FormCard.FormDelegateSeparator {}
        FormCard.FormButtonDelegate {
            text: i18nc("@action:button", "Check for Updates Now")
            icon.name: "view-refresh"
            onClicked: page.updates.checkNow()
        }
    }
}
