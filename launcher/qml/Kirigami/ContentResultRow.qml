// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * One result of Add Content: logo, name and author, one line of what it is,
 * and on the right what can be done -- Install (the newest version that
 * runs on this instance, dependencies included), its progress while that
 * runs, or Installed. Clicking the row opens the project, to pick another
 * version.
 *
 * `model` carries ContentProviderModel's roles (title, author, description,
 * logoUrl, installed, ...); `installer` is the instance's ContentInstaller.
 */
QQC2.ItemDelegate {
    id: row

    required property var model
    required property int index
    required property var installer

    readonly property string key: row.installer.projectKey(row.model.title || "", row.model.author || "")
    readonly property var download: row.installer.downloadFor(row.key)
    readonly property bool checking: row.installer.isChecking(row.key)
    readonly property bool downloading: !!row.download && row.download.running
    readonly property bool installed: row.model.installed === true || (!!row.download && row.download.succeeded)
    readonly property bool failed: !!row.download && row.download.failed && !row.downloading

    function project() {
        return {
            projectId: String(row.model.projectId || ""),
            title: row.model.title || "",
            author: row.model.author || "",
            description: row.model.description || "",
            logoUrl: row.model.logoUrl || "",
            installed: row.model.installed === true
        }
    }

    text: row.model.title || ""
    Accessible.description: row.model.description || ""

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        ProjectLogo {
            Layout.preferredWidth: Kirigami.Units.iconSizes.large
            Layout.preferredHeight: Kirigami.Units.iconSizes.large
            logoUrl: row.model.logoUrl || ""
            fallbackSource: "image://instanceicon/gear"
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing
                QQC2.Label {
                    Layout.maximumWidth: Math.ceil(implicitWidth)
                    Layout.fillWidth: true
                    text: row.text
                    font.bold: true
                    elide: Text.ElideRight
                }
                // The author keeps its room; the name gives way first.
                QQC2.Label {
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 12
                    visible: (row.model.author || "").length > 0
                    text: i18nc("@info %1 is a project's author", "by %1", row.model.author || "")
                    elide: Text.ElideRight
                    opacity: 0.7
                }
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: row.downloading ? (row.download.status || i18nc("@info:status", "Installing…"))
                    : row.checking ? i18nc("@info:status", "Checking what it needs…")
                    : row.failed ? (row.download.error || i18nc("@info:status", "The install failed."))
                    : (row.model.description || "")
                color: row.failed ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor
                opacity: row.failed ? 1 : 0.7
                elide: Text.ElideRight
            }
        }

        // What can be done with it, at the row's end.
        LaunchProgressBar {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 6
            visible: row.downloading || row.checking
            progress: row.downloading ? row.download.progress : -1
            Accessible.name: i18nc("@info:status", "Installing %1", row.text)
        }
        RowLayout {
            visible: row.installed && !row.downloading && !row.checking
            spacing: Kirigami.Units.smallSpacing
            Kirigami.Icon {
                implicitWidth: Kirigami.Units.iconSizes.small
                implicitHeight: Kirigami.Units.iconSizes.small
                source: "checkmark"
                color: Kirigami.Theme.positiveTextColor
            }
            QQC2.Label {
                text: i18nc("@info:status", "Installed")
                color: Kirigami.Theme.positiveTextColor
            }
        }
        QQC2.Button {
            visible: !row.installed && !row.downloading && !row.checking
            enabled: row.installer.blockedReason.length === 0
            text: row.failed ? i18nc("@action:button", "Retry") : i18nc("@action:button", "Install")
            icon.name: row.failed ? "view-refresh" : "download"
            onClicked: row.installer.quickInstall(row.index, row.project())
            QQC2.ToolTip.text: row.installer.blockedReason.length > 0 ? row.installer.blockedReason
                             : i18n("Installs the newest version that runs on this instance")
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
    }
}
