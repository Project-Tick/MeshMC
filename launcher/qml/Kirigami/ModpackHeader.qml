// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.coreaddons as KCoreAddons
import org.kde.kirigami as Kirigami

import "format.js" as Format

/*
 * The top of a modpack's page: its logo, name and author, how many have
 * downloaded and follow it and when it was last updated, what it is in a
 * line, and its categories. `pack` is the Discover result row.
 */
ColumnLayout {
    id: header

    property var pack: ({})
    readonly property var updatedDate: Format.parseDate(header.pack.updated)

    spacing: Kirigami.Units.largeSpacing * 2

    RowLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing * 2

        Kirigami.Icon {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: Kirigami.Units.iconSizes.enormous
            Layout.preferredHeight: Kirigami.Units.iconSizes.enormous
            source: header.pack.logoUrl || ""
            fallback: "package-x-generic"
            placeholder: "package-x-generic"
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                Layout.fillWidth: true
                level: 1
                text: header.pack.title || ""
                wrapMode: Text.Wrap
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: (header.pack.author || "").length > 0
                text: i18nc("@info %1 is a modpack's author", "by %1", header.pack.author || "")
                elide: Text.ElideRight
                opacity: 0.7
            }
            Flow {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.smallSpacing
                spacing: Kirigami.Units.largeSpacing * 2

                StatLabel {
                    visible: (header.pack.downloads || 0) > 0
                    iconName: "download"
                    text: i18ncp("@info %2 is a short number such as 12.3K", "%2 download", "%2 downloads",
                                 header.pack.downloads || 0, Format.compactNumber(header.pack.downloads))
                }
                StatLabel {
                    visible: (header.pack.follows || 0) > 0
                    iconName: "emblem-favorite"
                    text: i18ncp("@info %2 is a short number such as 12.3K", "%2 follower", "%2 followers",
                                 header.pack.follows || 0, Format.compactNumber(header.pack.follows))
                }
                StatLabel {
                    visible: !!header.updatedDate
                    iconName: "view-calendar"
                    text: header.updatedDate
                          ? i18nc("@info %1 is a date", "Updated %1",
                                  KCoreAddons.Format.formatRelativeDate(header.updatedDate, Locale.ShortFormat))
                          : ""
                }
            }
        }
    }

    QQC2.Label {
        Layout.fillWidth: true
        visible: text.length > 0
        text: header.pack.description || ""
        wrapMode: Text.Wrap
    }

    Flow {
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing
        visible: (header.pack.categories || []).length > 0
        Repeater {
            model: header.pack.categories || []
            Kirigami.Chip {
                required property string modelData
                text: modelData.length > 0 ? modelData.charAt(0).toUpperCase() + modelData.slice(1) : modelData
                closable: false
                interactive: false
            }
        }
    }

    component StatLabel: RowLayout {
        property string iconName
        property alias text: label.text
        spacing: Kirigami.Units.smallSpacing
        Kirigami.Icon {
            implicitWidth: Kirigami.Units.iconSizes.small
            implicitHeight: Kirigami.Units.iconSizes.small
            source: parent.iconName
        }
        QQC2.Label {
            id: label
        }
    }
}
