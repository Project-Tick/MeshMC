// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * A labelled slider in a form card, with the same properties as Kirigami
 * Addons' FormSliderDelegate. That one only arrived in Kirigami Addons
 * 1.15, and with an older one -- what most distributions ship -- a page
 * that names it fails to load, and with it the whole KDE interface. This
 * builds on AbstractFormDelegate, which every version has. The trailing
 * item is held in a plain Item rather than a LayoutItemProxy, which needs
 * Qt 6.6.
 */
FormCard.AbstractFormDelegate {
    id: root

    required property string label
    // Shown beside the label, such as the value in its unit.
    property string valueText: ""
    // Shown under the slider.
    property string description: ""
    // Shown after the slider, such as a warning icon.
    property Item trailing: null

    property alias value: slider.value
    property alias from: slider.from
    property alias to: slider.to
    property alias stepSize: slider.stepSize

    // Only for changes the user makes, by mouse, touch or keyboard.
    signal moved()

    focusPolicy: Qt.NoFocus
    background: null

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true
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

            QQC2.Label {
                visible: root.valueText.length > 0
                text: root.valueText
                color: root.enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                Accessible.ignored: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            QQC2.Slider {
                id: slider
                Layout.fillWidth: true
                Accessible.name: root.label
                Accessible.description: root.description
                onMoved: root.moved()
            }

            Item {
                visible: root.trailing !== null
                implicitWidth: root.trailing ? root.trailing.implicitWidth : 0
                implicitHeight: root.trailing ? root.trailing.implicitHeight : 0
                children: root.trailing ? [root.trailing] : []
            }
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
