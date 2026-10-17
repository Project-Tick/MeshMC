// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * New Instance's mod loader: the unmodified game or one of the four
 * loaders, each with its own logo, and then which build of it. The builds
 * listed are those for the Minecraft version picked above; the newest is
 * picked until another one is.
 */
FormCard.FormCard {
    id: card

    // NewInstanceController.
    required property var controller
    readonly property string loader: card.controller ? card.controller.loader : ""
    readonly property var loaderVersions: card.controller ? card.controller.loaderVersions : null

    // The controller's loader ids, "" for the unmodified game, with the
    // logo each wears (brands.js).
    readonly property var loaders: [
        { value: "", label: i18nc("@option:radio mod loader", "Vanilla"), brand: "vanilla" },
        { value: "fabric", label: "Fabric", brand: "fabric" },
        { value: "quilt", label: "Quilt", brand: "quilt" },
        { value: "forge", label: "Forge", brand: "forge" },
        { value: "neoforge", label: "NeoForge", brand: "neoforge" }
    ]

    function pick(value) {
        if (card.controller)
            card.controller.loader = value
    }

    FormCard.AbstractFormDelegate {
        background: null
        Accessible.role: Accessible.Grouping
        Accessible.name: i18nc("@label", "Mod loader")

        contentItem: RowLayout {
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: card.loaders
                delegate: QQC2.Button {
                    id: button
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    // The label is the content's own: the desktop style
                    // would draw a button's text a second time.
                    Accessible.name: button.modelData.label
                    topPadding: Kirigami.Units.largeSpacing
                    bottomPadding: Kirigami.Units.largeSpacing
                    checkable: true
                    checked: card.loader === button.modelData.value
                    onClicked: {
                        // A checkable button unchecks itself on a second
                        // click; the controller has the final say.
                        checked = Qt.binding(() => card.loader === button.modelData.value)
                        card.pick(button.modelData.value)
                    }
                    contentItem: ColumnLayout {
                        spacing: Kirigami.Units.smallSpacing
                        BrandIcon {
                            Layout.alignment: Qt.AlignHCenter
                            brand: button.modelData.brand
                            size: Kirigami.Units.iconSizes.medium
                            fallback: "applications-games"
                        }
                        QQC2.Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text: button.modelData.label
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }

    FormCard.FormDelegateSeparator {
        visible: card.loader.length > 0
    }
    FormCard.FormComboBoxDelegate {
        id: versionBox
        visible: card.loader.length > 0
        text: i18nc("@label:listbox", "Loader version")
        enabled: !!card.loaderVersions && count > 0
        model: card.loaderVersions
        textRole: "version"
        valueRole: "versionId"
        displayText: card.controller && card.controller.loaderLoading ? i18nc("@item:inlistbox", "Loading…")
                   : count === 0 ? i18nc("@item:inlistbox", "None for this Minecraft version")
                   : currentIndex >= 0 ? currentText
                   : card.controller ? card.controller.selectedLoaderVersion : ""
        // The controller may pick a build before its rows reach this box:
        // line the two up whenever either moves.
        function follow() {
            if (card.controller)
                versionBox.currentIndex = versionBox.indexOfValue(card.controller.selectedLoaderVersion)
        }
        onCountChanged: versionBox.follow()
        onActivated: card.controller.selectLoaderVersion(versionBox.currentValue)
        Connections {
            target: card.controller
            ignoreUnknownSignals: true
            function onSelectedLoaderVersionChanged() {
                versionBox.follow()
            }
        }
    }
}
