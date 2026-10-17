// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * About MeshMC: KDE's own About page (FormCard.AboutPage, from the
 * application's KAboutData), then what a bug report wants that it does not
 * show -- the platform, build date, commit and channels -- and the full
 * credits and licence texts (shell.about, AboutInfo, read from the build).
 * The build facts are selectable, to copy into a report.
 */
FormCard.AboutPage {
    id: page

    property var app: null
    readonly property var about: page.app && page.app.shell && page.app.shell.about ? page.app.shell.about : null

    // Shown in a dialog of its own: { title, text, format }.
    property var document: null

    component Fact: FormCard.FormTextDelegate {
        property string value
        visible: value.length > 0
        description: value
        descriptionItem.textFormat: Text.PlainText
    }

    FormCard.FormHeader {
        visible: !!page.about
        title: i18nc("@title:group", "This Build")
    }
    FormCard.FormCard {
        visible: !!page.about
        Fact {
            text: i18nc("@label", "Platform")
            value: page.about ? page.about.platform : ""
        }
        Fact {
            text: i18nc("@label", "Built")
            value: page.about ? page.about.buildDate : ""
        }
        Fact {
            text: i18nc("@label", "Commit")
            value: page.about ? page.about.commit : ""
            descriptionItem.font: Kirigami.Theme.fixedWidthFont
        }
        Fact {
            text: i18nc("@label", "Version channel")
            value: page.about ? page.about.channel : ""
        }
        Fact {
            text: i18nc("@label", "Update channel")
            value: page.about ? page.about.updateChannel : ""
        }
        FormCard.FormDelegateSeparator {}
        FormCard.FormButtonDelegate {
            visible: !!page.about && page.about.creditsHtml.length > 0
            text: i18nc("@action:button", "Full Credits")
            icon.name: "im-user"
            onClicked: page.showDocument(text, page.about.creditsHtml, Text.RichText)
        }
        FormCard.FormButtonDelegate {
            visible: !!page.about && page.about.licenseText.length > 0
            text: i18nc("@action:button", "Licenses of MeshMC and Its Libraries")
            icon.name: "license"
            onClicked: page.showDocument(text, page.about.licenseText, Text.MarkdownText)
        }
    }

    function showDocument(title, text, format) {
        page.document = { title: title, text: text, format: format }
        documentDialog.open()
    }

    Kirigami.Dialog {
        id: documentDialog
        title: page.document ? page.document.title : ""
        preferredWidth: Kirigami.Units.gridUnit * 36
        preferredHeight: Kirigami.Units.gridUnit * 30
        standardButtons: Kirigami.Dialog.Close
        onClosed: page.document = null

        QQC2.Label {
            padding: Kirigami.Units.largeSpacing
            text: page.document ? page.document.text : ""
            textFormat: page.document ? page.document.format : Text.PlainText
            wrapMode: Text.Wrap
            onLinkActivated: (link) => Qt.openUrlExternally(link)
        }
    }
}
