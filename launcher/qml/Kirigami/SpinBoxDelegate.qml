// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * A labelled spin box in a form card, laid out as Kirigami Addons'
 * FormSpinBoxDelegate is on the desktop. That one only says when the user
 * changed the value (valueModified) from Kirigami Addons 1.13 on; with an
 * older one -- Fedora 43 ships 1.12 -- a page that handles it fails to
 * load, and with it the whole KDE interface. Watching `value` instead would
 * also fire when a binding or the range moves it, and save a setting
 * nobody touched. This builds on AbstractFormDelegate, which every version
 * has.
 */
FormCard.AbstractFormDelegate {
    id: root

    required property string label
    // Shown under the spin box.
    property string description: ""

    property alias value: spinbox.value
    property alias from: spinbox.from
    property alias to: spinbox.to
    property alias stepSize: spinbox.stepSize
    property alias textFromValue: spinbox.textFromValue
    property alias valueFromText: spinbox.valueFromText

    // Only for changes the user makes, by mouse, wheel or keyboard.
    signal valueModified()

    focusPolicy: Qt.NoFocus
    background: null
    Accessible.description: root.description
    onClicked: spinbox.forceActiveFocus()

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            Layout.fillWidth: true
            text: root.label
            elide: Text.ElideRight
            color: root.enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
            wrapMode: Text.Wrap
            maximumLineCount: 2
            Accessible.ignored: true
        }

        QQC2.SpinBox {
            id: spinbox
            Layout.fillWidth: true
            locale: root.locale
            Accessible.name: root.label
            onValueModified: root.valueModified()
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: root.description.length > 0
            text: root.description
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.Wrap
            Accessible.ignored: true
        }
    }
}
