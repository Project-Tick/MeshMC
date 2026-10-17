// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls

/*
 * The "Updates" section of SettingsPage.qml: automatic update checks and a
 * manual "Check now". It stays out of the section list while this build has
 * no updater (UpdateSettingsController.available is false, or the
 * controller is not set up yet), the way SettingsPage's `shown` rule lets a
 * section hide itself.
 */
SettingsScroll {
    id: root

    readonly property var updates: LauncherStore.updates
    // Read by SettingsPage.qml's section list.
    readonly property bool shown: !!updates && updates.available
    // The same width SettingNumber gives its field.
    readonly property int intervalFieldWidth: 168

    title: i18n("Updates")
    description: i18n("Whether MeshMC looks for new versions by itself.")

    SettingsGroup {
        width: parent.width
        title: i18n("Update checks")
        SettingRow {
            label: i18n("Check for updates automatically")
            description: i18n("MeshMC asks the update server in the background and tells you when a new version is out.")
            Switch {
                checked: !!root.updates && root.updates.autoCheck
                Accessible.name: i18n("Check for updates automatically")
                onToggled: {
                    root.updates.autoCheck = checked
                    checked = Qt.binding(() => !!root.updates && root.updates.autoCheck)
                }
            }
        }
        SettingRow {
            label: i18n("How often")
            description: i18n("0 means only when MeshMC starts.")
            enabled: !!root.updates && root.updates.autoCheck
            SpinBox {
                id: interval
                width: root.intervalFieldWidth
                from: 0
                to: 24 * 30
                stepSize: 6
                editable: true
                value: root.updates ? root.updates.intervalHours : 0
                textFromValue: (value, locale) => value === 0 ? i18n("On launch")
                                                              : i18n("Every %1 h",
                                                                     Number(value).toLocaleString(locale, "f", 0))
                valueFromText: (text, locale) => {
                    var digits = text.replace(/[^0-9]/g, "")
                    return digits.length > 0 ? parseInt(digits, 10) : (root.updates ? root.updates.intervalHours : 0)
                }
                Accessible.name: i18n("Hours between update checks")
                onValueModified: {
                    root.updates.intervalHours = value
                    value = Qt.binding(() => root.updates ? root.updates.intervalHours : 0)
                }
            }
        }
        SettingRow {
            label: i18n("Check now")
            description: i18n("Looks for a new version right away.")
            Button {
                text: i18n("Check for updates")
                icon.source: Icons.url("refresh")
                onClicked: root.updates.checkNow()
            }
        }
    }
}
