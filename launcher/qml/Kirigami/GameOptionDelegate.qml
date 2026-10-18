// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * One options.txt entry of the Game Options tab, with the control the
 * game's own Options screen uses for it. `spec` says which (see
 * GameOptionsTab's `optionGroups`):
 *
 *   kind "slider"  from/to/step in the shown unit, toSlider(raw) -> number,
 *                  toRaw(number) -> string, text(number) -> label
 *   kind "switch"  "true"/"false"
 *   kind "choice"  choices: [{ value, label }]
 *   kind "text"    text() -> label; read-only
 *
 * A change is written with GameOptions.setValue() as it is made (a slider a
 * moment after it stops moving); the controls then bind back to what the
 * file says, so a refused write shows the old value again. A value the
 * control cannot represent (another game version's format, a GUI scale past
 * the list) is shown as written, read-only, rather than misread.
 */
Loader {
    id: delegate

    property var spec: ({})
    property string raw: ""
    // The GameOptions model (not the search proxy over it).
    property var store: null
    property bool editable: false

    readonly property int commitDelay: 400
    readonly property bool understood: spec.kind === "slider" ? raw !== "" && !isNaN(Number(raw))
                                     : spec.kind === "switch" ? raw === "true" || raw === "false"
                                     : spec.kind === "choice" ? spec.choices.some(c => c.value === raw)
                                     : true
    readonly property bool interactive: understood && spec.kind !== "text"

    function commit(value) {
        if (delegate.store && value !== delegate.raw)
            delegate.store.setValue(delegate.spec.key, value)
    }

    Layout.fillWidth: true
    enabled: delegate.editable || !delegate.interactive
    sourceComponent: !delegate.interactive ? textControl
                   : delegate.spec.kind === "slider" ? sliderControl
                   : delegate.spec.kind === "switch" ? switchControl
                   : choiceControl

    Component {
        id: sliderControl
        SliderDelegate {
            id: slider
            label: delegate.spec.label
            from: delegate.spec.from
            to: delegate.spec.to
            stepSize: delegate.spec.step
            value: delegate.spec.toSlider(delegate.raw)
            valueText: delegate.spec.text(Math.round(slider.value))
            onMoved: commitTimer.restart()

            Timer {
                id: commitTimer
                interval: delegate.commitDelay
                onTriggered: {
                    delegate.commit(delegate.spec.toRaw(Math.round(slider.value)))
                    slider.value = Qt.binding(() => delegate.spec.toSlider(delegate.raw))
                }
            }
        }
    }

    Component {
        id: switchControl
        FormCard.FormSwitchDelegate {
            text: delegate.spec.label
            checked: delegate.raw === "true"
            onToggled: {
                delegate.commit(checked ? "true" : "false")
                checked = Qt.binding(() => delegate.raw === "true")
            }
        }
    }

    Component {
        id: choiceControl
        FormCard.FormComboBoxDelegate {
            text: delegate.spec.label
            model: delegate.spec.choices
            textRole: "label"
            valueRole: "value"
            // Through `count`, so it is worked out again once the model
            // has arrived rather than once, before it.
            currentIndex: count > 0 ? indexOfValue(delegate.raw) : -1
            onActivated: {
                delegate.commit(currentValue)
                currentIndex = Qt.binding(() => count > 0 ? indexOfValue(delegate.raw) : -1)
            }
        }
    }

    Component {
        id: textControl
        FormCard.FormTextDelegate {
            text: delegate.spec.label
            description: delegate.spec.kind === "text" ? delegate.spec.text() : delegate.raw
        }
    }
}
