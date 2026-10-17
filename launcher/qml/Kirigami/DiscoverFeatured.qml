// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * A row of pictures above Discover's list while nothing is searched for:
 * the first results that carry a picture of the pack (the models' galleryUrl
 * role -- Modrinth's featured image, a screenshot, FTB's splash art), as
 * cards to click. It is only a different view of the results the page has
 * already loaded, so it asks the network for nothing but the pictures, and
 * it shows nothing at all unless there are enough of them to be a row
 * (ATLauncher has none).
 */
Item {
    id: strip

    // The provider's model, whose first rows are looked at.
    property var source
    // Fewer pictures than this is no row; only the first `limit` results are
    // looked at, since further down is no longer a highlight.
    readonly property int minimum: 3
    readonly property int limit: 10
    // How many of those have a picture; counted when a row appears, since
    // the Repeater cannot be asked which of its cards are shown.
    property int pictures: 0
    readonly property bool shown: strip.pictures >= strip.minimum

    // A result's row, as the page's open() takes it.
    signal opened(var row)

    function recount() {
        var found = 0
        for (var i = 0; i < Math.min(cards.count, strip.limit); ++i) {
            var loader = cards.itemAt(i)
            if (loader && loader.active)
                ++found
        }
        strip.pictures = found
    }

    visible: strip.shown
    implicitHeight: strip.shown ? column.implicitHeight + Kirigami.Units.largeSpacing : 0

    ColumnLayout {
        id: column
        width: parent.width
        spacing: Kirigami.Units.smallSpacing

        Kirigami.Heading {
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.topMargin: Kirigami.Units.largeSpacing
            level: 3
            text: i18nc("@title:group a row of highlighted modpacks", "Featured")
        }
        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 13 + Kirigami.Units.smallSpacing
            contentHeight: Kirigami.Units.gridUnit * 12
            // The row scrolls sideways only; a wheel turns the page.
            QQC2.ScrollBar.vertical.policy: QQC2.ScrollBar.AlwaysOff

            Row {
                x: Kirigami.Units.largeSpacing
                spacing: Kirigami.Units.largeSpacing

                // One loader per result, but a card only for the first few
                // that have a picture.
                Repeater {
                    id: cards
                    model: strip.source
                    onItemAdded: Qt.callLater(strip.recount)
                    onItemRemoved: Qt.callLater(strip.recount)
                    delegate: Loader {
                        id: slot
                        required property int index
                        required property var model
                        active: slot.index < strip.limit && !!slot.model.galleryUrl
                        visible: slot.active
                        // Not Kirigami.Card, whose banner is as tall as its
                        // picture is: a tall one would push the title out of
                        // the card.
                        sourceComponent: Kirigami.AbstractCard {
                            width: Kirigami.Units.gridUnit * 16
                            height: Kirigami.Units.gridUnit * 12
                            showClickFeedback: true
                            onClicked: strip.opened(slot.model)
                            Accessible.name: slot.model.title || ""
                            contentItem: ColumnLayout {
                                spacing: Kirigami.Units.smallSpacing
                                Image {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: Kirigami.Units.gridUnit * 7
                                    source: slot.model.galleryUrl || ""
                                    sourceSize.width: width * 2
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                    Accessible.ignored: true
                                }
                                Kirigami.Heading {
                                    Layout.fillWidth: true
                                    level: 4
                                    text: slot.model.title || ""
                                    elide: Text.ElideRight
                                }
                                QQC2.Label {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    text: slot.model.description || ""
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                    opacity: 0.7
                                    verticalAlignment: Text.AlignTop
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
