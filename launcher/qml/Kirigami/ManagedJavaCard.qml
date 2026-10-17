// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * The Java runtimes MeshMC downloaded itself (Settings, Java & Memory):
 * which one, where it lives, whether it is in use, and buttons to use or
 * remove it. Removing deletes the runtime's folder, so it asks first.
 * Download Java and Refresh head the list.
 */
ColumnLayout {
    id: card

    required property var source
    // JavaController and JavaDownloadController.
    property var java: null
    property var download: null
    readonly property var runtimes: card.java ? card.java.managed : []
    readonly property string currentPath: card.source.string("JavaPath")

    // A message for the window's passive notification.
    signal notify(string message)

    Layout.fillWidth: true
    spacing: 0
    visible: !!card.java && card.java.downloaderAvailable

    Connections {
        target: card.download
        ignoreUnknownSignals: true
        function onInstalled() {
            if (card.java)
                card.java.refreshManaged()
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Downloaded Java")
        actions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Download Java…")
                icon.name: "download"
                enabled: !!card.download
                onTriggered: downloadDialog.open()
            },
            Kirigami.Action {
                text: i18nc("@action:button", "Refresh")
                icon.name: "view-refresh"
                displayHint: Kirigami.DisplayHint.IconOnly
                onTriggered: card.java.refreshManaged()
            }
        ]
    }

    FormCard.FormCard {
        autoSeparators: true
        FormCard.FormTextDelegate {
            visible: card.runtimes.length === 0
            text: i18n("Nothing downloaded yet.")
            description: i18n("Java that MeshMC downloads stays in its own folder.")
        }
        Repeater {
            model: card.runtimes
            delegate: FormCard.FormTextDelegate {
                id: runtime
                required property var modelData
                readonly property bool inUse: card.currentPath === runtime.modelData.javaPath
                text: i18nc("@item %1 runtime name, %2 vendor", "%1 (%2)", runtime.modelData.name, runtime.modelData.vendor)
                description: runtime.modelData.javaPath
                descriptionItem.elide: Text.ElideMiddle
                descriptionItem.wrapMode: Text.NoWrap
                trailing: RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    QQC2.Label {
                        visible: runtime.inUse
                        text: i18nc("@info:status", "In use")
                        color: Kirigami.Theme.positiveTextColor
                    }
                    QQC2.Button {
                        visible: !runtime.inUse
                        text: i18nc("@action:button", "Use")
                        onClicked: card.source.setValue("JavaPath", runtime.modelData.javaPath)
                    }
                    QQC2.Button {
                        icon.name: "edit-delete"
                        text: i18nc("@action:button", "Remove…")
                        display: QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.text: text
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        onClicked: {
                            removeDialog.target = runtime.modelData
                            removeDialog.open()
                        }
                    }
                }
            }
        }
    }

    Kirigami.PromptDialog {
        id: removeDialog
        property var target: null
        title: i18nc("@title:dialog", "Remove This Java?")
        subtitle: removeDialog.target
                  ? i18n("%1 will be deleted from this computer. Instances that use it go back to automatic Java.",
                         removeDialog.target.name)
                  : ""
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Remove")
                icon.name: "edit-delete"
                onTriggered: {
                    var error = card.java.removeManaged(removeDialog.target.root)
                    removeDialog.close()
                    if (error.length > 0)
                        card.notify(error)
                }
            }
        ]
    }

    JavaDownloadDialog {
        id: downloadDialog
        download: card.download
        onUseRequested: (path) => card.source.setValue("JavaPath", path)
    }
}
