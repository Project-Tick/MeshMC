// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * Add Content's filters: Minecraft version, mod loader, where it runs,
 * categories, and whether to show what is already installed -- what the
 * widget download dialog's filter panel offered. They start on this
 * instance's own version and loader. Each change searches again at once
 * (ContentBrowser re-runs the search itself), so there is no Apply; Reset
 * goes back to where they started.
 *
 * Groups a provider cannot answer (where it runs and the licence on
 * CurseForge, loaders for resource packs) are left out rather than shown
 * doing nothing. One control per kind of choice: check boxes where several
 * can be on, radio buttons where exactly one is, a drop-down for the long
 * version list.
 */
Kirigami.Dialog {
    id: dialog

    // ContentBrowser.
    property var browser: null

    // Categories and loaders in two columns: short words, and one column
    // of twenty categories would make the dialog scroll at once.
    readonly property int optionColumns: 2

    title: i18nc("@title:dialog", "Filters")
    preferredWidth: Kirigami.Units.gridUnit * 28
    maximumHeight: applicationWindow().height - Kirigami.Units.gridUnit * 4
    standardButtons: Kirigami.Dialog.Close
    customFooterActions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Reset")
            icon.name: "edit-undo"
            enabled: !!dialog.browser && dialog.browser.activeFilterCount > 0
            onTriggered: dialog.browser.resetFilters()
        }
    ]

    onOpened: if (dialog.browser) dialog.browser.loadFilterOptions()

    function toggled(list, id, on) {
        var out = list.filter(x => x !== id)
        if (on)
            out.push(id)
        return out
    }

    Kirigami.FormLayout {
        QQC2.ComboBox {
            id: versionBox
            Kirigami.FormData.label: i18nc("@label:listbox", "Minecraft version:")
            Layout.preferredWidth: Kirigami.Units.gridUnit * 12
            readonly property var options: [{ value: "", text: i18nc("@item:inlistbox", "Any version") }]
                .concat((dialog.browser ? dialog.browser.gameVersionOptions : []).map(v => ({ value: v, text: v })))
            model: versionBox.options
            textRole: "text"
            valueRole: "value"
            currentIndex: dialog.browser ? Math.max(0, versionBox.options.findIndex(o => o.value === dialog.browser.gameVersionFilter)) : 0
            onActivated: {
                dialog.browser.gameVersionFilter = currentValue
                currentIndex = Qt.binding(() => dialog.browser ? Math.max(0, versionBox.options.findIndex(o => o.value === dialog.browser.gameVersionFilter)) : 0)
            }
        }

        GridLayout {
            Kirigami.FormData.label: i18nc("@label", "Mod loader:")
            Kirigami.FormData.buddyFor: loaderRepeater.count > 0 ? loaderRepeater.itemAt(0) : null
            visible: !!dialog.browser && dialog.browser.loaderFilterAvailable
            columns: dialog.optionColumns
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: 0
            Repeater {
                id: loaderRepeater
                model: dialog.browser ? dialog.browser.loaderOptions : []
                QQC2.CheckBox {
                    required property var modelData
                    text: modelData.label
                    checked: dialog.browser.loaderFilter.indexOf(modelData.id) >= 0
                    onToggled: {
                        dialog.browser.loaderFilter = dialog.toggled(dialog.browser.loaderFilter, modelData.id, checked)
                        checked = Qt.binding(() => dialog.browser.loaderFilter.indexOf(modelData.id) >= 0)
                    }
                }
            }
        }

        GridLayout {
            Kirigami.FormData.label: i18nc("@label where a mod runs", "Runs on:")
            Kirigami.FormData.buddyFor: environmentRepeater.count > 0 ? environmentRepeater.itemAt(0) : null
            visible: !!dialog.browser && dialog.browser.extendedFilters && dialog.browser.loaderFilterAvailable
            columns: dialog.optionColumns
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: 0
            QQC2.ButtonGroup {
                id: environmentGroup
            }
            Repeater {
                id: environmentRepeater
                model: [
                    { value: "", text: i18nc("@option:radio a mod that runs", "Anywhere") },
                    { value: "client", text: i18nc("@option:radio a mod that runs", "Client") },
                    { value: "server", text: i18nc("@option:radio a mod that runs", "Server") },
                    { value: "both", text: i18nc("@option:radio a mod that runs", "Client and server") }
                ]
                QQC2.RadioButton {
                    required property var modelData
                    text: modelData.text
                    QQC2.ButtonGroup.group: environmentGroup
                    checked: !!dialog.browser && dialog.browser.environmentFilter === modelData.value
                    onToggled: {
                        dialog.browser.environmentFilter = modelData.value
                        checked = Qt.binding(() => !!dialog.browser && dialog.browser.environmentFilter === modelData.value)
                    }
                }
            }
        }

        GridLayout {
            Kirigami.FormData.label: i18nc("@label", "Categories:")
            Kirigami.FormData.buddyFor: categoryRepeater.count > 0 ? categoryRepeater.itemAt(0) : null
            columns: dialog.optionColumns
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: 0
            Repeater {
                id: categoryRepeater
                model: dialog.browser ? dialog.browser.categories : []
                QQC2.CheckBox {
                    required property var modelData
                    text: modelData.label
                    checked: dialog.browser.categoryFilter.indexOf(modelData.id) >= 0
                    onToggled: {
                        dialog.browser.categoryFilter = dialog.toggled(dialog.browser.categoryFilter, modelData.id, checked)
                        checked = Qt.binding(() => dialog.browser.categoryFilter.indexOf(modelData.id) >= 0)
                    }
                }
            }
            QQC2.Label {
                visible: categoryRepeater.count === 0
                text: i18nc("@info:status", "Loading categories…")
                opacity: 0.7
            }
        }

        QQC2.CheckBox {
            Kirigami.FormData.label: i18nc("@label", "Show:")
            text: i18nc("@option:check", "Hide what is already installed")
            checked: !!dialog.browser && dialog.browser.hideInstalled
            onToggled: {
                dialog.browser.hideInstalled = checked
                checked = Qt.binding(() => !!dialog.browser && dialog.browser.hideInstalled)
            }
        }
        QQC2.CheckBox {
            visible: !!dialog.browser && dialog.browser.extendedFilters
            text: i18nc("@option:check", "Open source only")
            checked: !!dialog.browser && dialog.browser.openSourceOnly
            onToggled: {
                dialog.browser.openSourceOnly = checked
                checked = Qt.binding(() => !!dialog.browser && dialog.browser.openSourceOnly)
            }
        }
    }
}
