// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls
import MeshMC.Theme

/*
 * The "Appearance" section of SettingsPage.qml: the launcher's own
 * language, palette and mode, motion, and the cat.
 */
SettingsScroll {
    id: root

    // TranslationsModel (languageKey, name, completeness) and the function
    // that switches to one of its keys, live.
    property var languages: null
    property var selectLanguage: null
    // Whether launcher/qml/Cat was built at all (Main.qml's own
    // catAvailable, in turn MESHMC_HAS_CAT) -- hides the Cat group below
    // rather than offering switches that would do nothing on a build
    // without Qt Quick3D.
    property bool catAvailable: false

    // The language box, as wide as the longest language name needs.
    readonly property int languageBoxWidth: 260

    // One tile in the cat variant picker below -- the coat's face, cropped
    // out of the same texture the 3D cat wears (see Cat/CatRig.qml), plus a label,
    // styled like SkinCapeEditor's CapeTile but simple enough not to
    // warrant its own file for the one row that uses it.
    component CatVariantTile: Item {
        id: tile
        required property string value
        required property string label
        readonly property bool selected: SettingsStore.string("CatVariant") === tile.value

        // The face is the texture's 5x4 pixels at seven times their size,
        // on a swatch with room around it, over its label.
        readonly property int faceScale: 7
        readonly property int swatchSize: 48
        implicitWidth: swatchSize + Theme.space.sm
        implicitHeight: swatch.height + Theme.space.xs + tileLabel.implicitHeight

        Rectangle {
            id: swatch
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            width: tile.swatchSize
            height: tile.swatchSize
            radius: Theme.radius.md
            color: tile.selected ? Theme.palette.accentSubtle : Theme.palette.surface
            border.width: tile.selected ? 2 : 1
            border.color: tile.selected ? Theme.palette.accent : Theme.palette.border

            // The front of the head in the model's 64x64 UV layout: five
            // pixels wide, four high, eyes on its second row.
            Image {
                anchors.centerIn: parent
                width: 5 * tile.faceScale
                height: 4 * tile.faceScale
                smooth: false
                sourceClipRect: Qt.rect(5, 33, 5, 4)
                source: "qrc:/qt/qml/MeshMC/Cat/textures/cat_" + tile.value + ".png"
            }
        }

        Text {
            id: tileLabel
            anchors.top: swatch.bottom
            anchors.topMargin: Theme.space.xs
            anchors.horizontalCenter: parent.horizontalCenter
            text: tile.label
            color: Theme.palette.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.type.caption.pixelSize
        }

        TapHandler {
            onTapped: SettingsStore.setValue("CatVariant", tile.value)
        }
    }

    title: i18n("Appearance")
    description: i18n("The launcher's own language and colour scheme.")
    SettingsGroup {
        width: parent.width
        title: i18n("Language")
        SettingRow {
            label: i18n("Display language")
            description: i18n("Applied immediately.")
            ComboBox {
                id: languageBox
                width: root.languageBoxWidth
                model: root.languages
                textRole: "name"
                valueRole: "languageKey"
                enabled: !!root.languages && !!root.selectLanguage
                // Follows the setting (changed here, in the old settings
                // window or by a plugin), not only the first time.
                readonly property string stored: SettingsStore.string("Language")
                function syncToSetting() {
                    currentIndex = indexOfValue(stored)
                }
                onStoredChanged: syncToSetting()
                onCountChanged: syncToSetting()
                Component.onCompleted: syncToSetting()
                onActivated: {
                    root.selectLanguage(currentValue)
                    syncToSetting()
                }
                Accessible.name: i18n("Display language")
            }
        }
    }
    SettingsGroup {
        width: parent.width
        title: i18n("Theme")
        SettingRow {
            wide: true
            label: i18n("Palette")
            description: i18n("The launcher's colours. System uses your desktop's own; the others each have a dark and a light variant.")
            PalettePicker {
                current: Theme.scheme
                onPicked: (scheme) => {
                    Theme.scheme = scheme
                    SettingsStore.setValue("UiPalette", scheme)
                }
            }
        }
        SettingChoice {
            key: "UiThemeMode"
            label: i18n("Mode")
            // The System palette is light or dark with the desktop, so a
            // mode of its own would have nothing to switch.
            enabled: Theme.scheme !== "system"
            description: enabled ? "" : i18n("Follows your desktop while the palette is System.")
            showDivider: true
            options: [
                { value: "system", label: i18n("System") },
                { value: "dark", label: i18n("Dark") },
                { value: "light", label: i18n("Light") }
            ]
            onChanged: (value) => Theme.mode = value
        }
    }
    SettingsGroup {
        width: parent.width
        title: i18n("Font")
        FontFamilyRow {
            key: "UiFontFamily"
            defaultFamily: "Inter"
            label: i18n("Interface font")
            description: i18n("The font every screen of the launcher is drawn in. Sizes stay the same. Inter comes with MeshMC; the rest are installed on this computer.")
        }
    }
    SettingsGroup {
        width: parent.width
        title: i18n("Motion")
        SettingSwitch {
            key: "UiReduceMotion"
            label: i18n("Reduce motion")
            description: i18n("Turns off decorative animation across the launcher, including the cat.")
        }
    }
    // Hidden outright rather than shown disabled: a build
    // without Qt Quick3D has nothing these switches could turn
    // on, and a visible-but-inert row reads as broken, not as
    // "not for you". See root.catAvailable's own comment.
    SettingsGroup {
        width: parent.width
        visible: root.catAvailable
        title: i18n("Cat")
        SettingSwitch {
            key: "CatEnabled"
            label: i18n("Show the cat")
            description: i18n("A small Minecraft cat that lives on the play bar. Click to pet it, double-click and it hops.")
        }
        SettingRow {
            wide: true
            showDivider: true
            label: i18n("Variant")
            Row {
                spacing: Theme.space.md
                CatVariantTile { value: "calico"; label: i18n("Calico") }
                CatVariantTile { value: "ginger"; label: i18n("Ginger") }
                CatVariantTile { value: "black"; label: i18n("Black") }
                CatVariantTile { value: "white"; label: i18n("White") }
                CatVariantTile { value: "siamese"; label: i18n("Siamese") }
            }
        }
    }
}
