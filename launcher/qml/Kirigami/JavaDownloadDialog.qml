// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "combos.js" as Combos

/*
 * Download a Java runtime: a vendor, a Java version and a build for this
 * computer, then its progress. The work is JavaDownloadController's; the
 * runtime lands in MeshMC's own Java folder, so it shows up under
 * "Downloaded Java" and in the Java picker afterwards.
 */
Kirigami.Dialog {
    id: dialog

    // JavaDownloadController.
    property var download: null
    readonly property bool busy: !!dialog.download && dialog.download.downloading
    readonly property bool done: !!dialog.download && !dialog.download.downloading
                                 && dialog.download.installedPath.length > 0

    // Use the Java just downloaded; the caller writes the setting.
    signal useRequested(string path)

    title: i18nc("@title:dialog", "Download Java")
    padding: Kirigami.Units.largeSpacing
    preferredWidth: Kirigami.Units.gridUnit * 26
    closePolicy: dialog.busy ? QQC2.Popup.NoAutoClose : QQC2.Popup.CloseOnEscape
    standardButtons: Kirigami.Dialog.NoButton

    onOpened: if (dialog.download) dialog.download.reset()
    onClosed: if (dialog.download) dialog.download.cancel()

    customFooterActions: [
        Kirigami.Action {
            visible: !dialog.done
            text: !!dialog.download && dialog.download.alreadyInstalled ? i18nc("@action:button", "Download Again")
                                                                        : i18nc("@action:button", "Download")
            icon.name: "download"
            enabled: !!dialog.download && dialog.download.build >= 0 && !dialog.busy && !dialog.download.loading
            onTriggered: dialog.download.download()
        },
        Kirigami.Action {
            visible: dialog.done
            text: i18nc("@action:button", "Use This Java")
            icon.name: "dialog-ok-apply"
            onTriggered: {
                dialog.useRequested(dialog.download.installedPath)
                dialog.close()
            }
        },
        Kirigami.Action {
            text: dialog.busy ? i18nc("@action:button", "Cancel") : i18nc("@action:button", "Close")
            icon.name: dialog.busy ? "dialog-cancel" : "dialog-close"
            onTriggered: dialog.close()
        }
    ]

    // A combo box over one of the controller's lists, selected by value
    // (again whenever the list or the choice changes: the box selects its
    // first row by itself when a list arrives).
    component Choice: QQC2.ComboBox {
        id: box
        property var chosen
        property bool byIndex: false
        signal picked(var value)
        textRole: "label"
        valueRole: box.byIndex ? "" : "value"
        function sync() { currentIndex = count > 0 ? (box.byIndex ? box.chosen : indexOfValue(box.chosen)) : -1 }
        onChosenChanged: sync()
        onCountChanged: sync()
        Component.onCompleted: {
            sync()
            // A long list scrolls under the box (combos.js).
            Combos.capPopup(box, Kirigami.Units.gridUnit)
        }
        onActivated: (index) => {
            box.picked(box.byIndex ? index : currentValue)
            sync()
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.fillWidth: true
            Layout.preferredWidth: Kirigami.Units.gridUnit * 20
            text: i18n("MeshMC keeps the Java it downloads in its own folder, apart from any you installed yourself.")
            wrapMode: Text.Wrap
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true
            enabled: !dialog.busy

            Choice {
                Kirigami.FormData.label: i18nc("@label:listbox", "Vendor:")
                model: dialog.download ? dialog.download.vendors : []
                chosen: dialog.download ? dialog.download.vendor : ""
                onPicked: (value) => dialog.download.selectVendor(value)
            }
            Choice {
                Kirigami.FormData.label: i18nc("@label:listbox", "Java version:")
                model: dialog.download ? dialog.download.versions : []
                chosen: dialog.download ? dialog.download.version : ""
                displayText: currentIndex >= 0 ? currentText
                                               : (dialog.download && dialog.download.loading ? i18nc("@item", "Loading…") : "")
                onPicked: (value) => dialog.download.selectVersion(value)
            }
            Choice {
                Kirigami.FormData.label: i18nc("@label:listbox", "Build:")
                byIndex: true
                model: dialog.download ? dialog.download.builds : []
                chosen: dialog.download ? dialog.download.build : -1
                displayText: currentIndex >= 0 ? currentText : ""
                onPicked: (row) => dialog.download.selectBuild(row)
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: !!dialog.download && dialog.download.alreadyInstalled && !dialog.busy && !dialog.done
            text: i18n("This build is already downloaded. Downloading it again replaces it.")
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: !!dialog.download && dialog.download.error.length > 0
            type: Kirigami.MessageType.Error
            text: dialog.download ? dialog.download.error : ""
        }
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: dialog.done
            type: Kirigami.MessageType.Positive
            text: i18n("Java is downloaded and ready.")
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: dialog.busy
            QQC2.ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 1
                value: dialog.download ? dialog.download.progress : 0
                indeterminate: !!dialog.download && dialog.download.progress <= 0
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: dialog.download ? dialog.download.status : ""
                elide: Text.ElideRight
            }
        }
    }
}
