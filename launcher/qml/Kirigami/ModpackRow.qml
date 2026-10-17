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
 * One modpack in Discover's results: its logo, name and author, two lines
 * of what it is, then how many have downloaded it and when it was last
 * updated -- what a catalogue's list shows. Clicking it opens the pack.
 *
 * `model` carries the modpack models' roles (projectId, title, author,
 * description, logoUrl, downloads, updated, ...); every provider has them,
 * though not every provider fills them all (ATLauncher counts no downloads).
 */
QQC2.ItemDelegate {
    id: row

    required property var model
    required property int index

    readonly property int downloads: row.model.downloads || 0
    readonly property var updatedDate: Format.parseDate(row.model.updated)

    text: row.model.title || ""
    Accessible.description: row.model.description || ""

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        ProjectLogo {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
            Layout.preferredHeight: Kirigami.Units.iconSizes.huge
            logoUrl: row.model.logoUrl || ""
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            // A layout's maximum width is its children's: with only a name in
            // it (Technic's search hits carry no description, author or
            // date) the column would stop at the name's width and be centred
            // in the row. It takes the whole row.
            Layout.maximumWidth: Number.POSITIVE_INFINITY
            spacing: Kirigami.Units.smallSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Heading {
                    Layout.maximumWidth: Math.ceil(implicitWidth)
                    Layout.fillWidth: true
                    level: 4
                    text: row.text
                    elide: Text.ElideRight
                }
                // The author keeps its room; the name gives way first.
                QQC2.Label {
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 12
                    visible: (row.model.author || "").length > 0
                    text: i18nc("@info %1 is a modpack's author", "by %1", row.model.author || "")
                    elide: Text.ElideRight
                    opacity: 0.7
                }
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: row.model.description || ""
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
            RowLayout {
                spacing: Kirigami.Units.smallSpacing
                opacity: 0.7
                visible: row.downloads > 0 || !!row.updatedDate

                Kirigami.Icon {
                    visible: row.downloads > 0
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                    source: "download-symbolic"
                }
                QQC2.Label {
                    visible: row.downloads > 0
                    text: i18ncp("@info %2 is a short number such as 12.3K", "%2 download", "%2 downloads",
                                 row.downloads, Format.compactNumber(row.downloads))
                }
                Item {
                    visible: row.downloads > 0 && !!row.updatedDate
                    implicitWidth: Kirigami.Units.largeSpacing
                }
                Kirigami.Icon {
                    visible: !!row.updatedDate
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                    source: "view-calendar-symbolic"
                }
                QQC2.Label {
                    visible: !!row.updatedDate
                    text: row.updatedDate
                          ? i18nc("@info %1 is a date", "Updated %1",
                                  KCoreAddons.Format.formatRelativeDate(row.updatedDate, Locale.ShortFormat))
                          : ""
                }
            }
        }
    }
}
