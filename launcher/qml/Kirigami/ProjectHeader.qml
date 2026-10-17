// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * The top of a modpack's or a mod's page, laid out the way KDE's software
 * centre lays out an application's: across the page, in the background
 * colour tinted with the logo's own colour, the logo big beside the name,
 * the author and a line of figures; under them what to do -- Install and
 * its choices, `controls` -- and on the right a column of
 * details (where it is from, what it runs on, ...), which drops under the
 * rest on a narrow page.
 *
 * The caller places it edge to edge (negative margins in a ScrollablePage).
 */
Rectangle {
    id: header

    // The logo; `fallbackSource` (one of the launcher's own pictures) when
    // there is none or it cannot be loaded.
    property string iconSource
    property string fallbackSource: "image://instanceicon/planks"
    property string title
    property string author
    // A line of figures under the author: [{iconName, text}].
    property var stats: []
    // The details column: [{label, text, brand}], `brand` an optional
    // brands.js id drawn before the text.
    property var details: []
    // The shell's BrandLogos, for `details` brands fetched on first use.
    property var logos: null
    // The page's own padding, kept inside the band.
    property real horizontalPadding: Kirigami.Units.gridUnit
    // What to do with it, under the name: Install and its choices.
    property alias controls: controlsColumn.data

    readonly property bool stacked: header.width < Kirigami.Units.gridUnit * 40

    implicitHeight: layout.implicitHeight + Kirigami.Units.gridUnit * 2
    color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, logoColors.dominant, 0.1)

    Kirigami.ImageColors {
        id: logoColors
        source: logo
        fallbackDominant: Kirigami.Theme.backgroundColor
    }

    GridLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: header.horizontalPadding
        anchors.rightMargin: header.horizontalPadding
        anchors.topMargin: Kirigami.Units.gridUnit
        anchors.bottomMargin: Kirigami.Units.gridUnit
        columns: header.stacked ? 1 : 2
        columnSpacing: Kirigami.Units.gridUnit * 2
        rowSpacing: Kirigami.Units.gridUnit

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: Kirigami.Units.largeSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.gridUnit

                ProjectLogo {
                    id: logo
                    Layout.alignment: Qt.AlignTop
                    implicitWidth: Kirigami.Units.iconSizes.huge
                    implicitHeight: Kirigami.Units.iconSizes.huge
                    logoUrl: header.iconSource
                    fallbackSource: header.fallbackSource
                    onLoaded: logoColors.update()
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: 0

                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 1
                        type: Kirigami.Heading.Type.Primary
                        text: header.title
                        wrapMode: Text.Wrap
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: header.author.length > 0
                        text: i18nc("@info %1 is a project's author", "by %1", header.author)
                        elide: Text.ElideRight
                        opacity: 0.7
                    }
                    Flow {
                        Layout.fillWidth: true
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        visible: header.stats.length > 0
                        spacing: Kirigami.Units.largeSpacing * 2
                        Repeater {
                            model: header.stats
                            RowLayout {
                                required property var modelData
                                spacing: Kirigami.Units.smallSpacing
                                Kirigami.Icon {
                                    implicitWidth: Kirigami.Units.iconSizes.small
                                    implicitHeight: Kirigami.Units.iconSizes.small
                                    source: modelData.iconName
                                }
                                QQC2.Label {
                                    text: modelData.text
                                }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                id: controlsColumn
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
            }
        }

        // The details, label beside value, labels right-aligned as in a form.
        GridLayout {
            Layout.alignment: header.stacked ? Qt.AlignLeft | Qt.AlignTop : Qt.AlignRight | Qt.AlignTop
            Layout.maximumWidth: header.stacked ? -1 : Kirigami.Units.gridUnit * 20
            visible: header.details.length > 0
            columns: 2
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: Kirigami.Units.smallSpacing

            Repeater {
                model: header.details
                delegate: QQC2.Label {
                    required property var modelData
                    required property int index
                    Layout.row: index
                    Layout.column: 0
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    text: modelData.label
                    opacity: 0.7
                }
            }
            Repeater {
                model: header.details
                delegate: RowLayout {
                    required property var modelData
                    required property int index
                    Layout.row: index
                    Layout.column: 1
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: Kirigami.Units.smallSpacing
                    BrandIcon {
                        visible: (modelData.brand || "").length > 0
                        brand: modelData.brand || ""
                        logos: header.logos
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: modelData.text
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }
}
