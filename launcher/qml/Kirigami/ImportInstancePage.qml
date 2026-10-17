// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * "Import…": an instance from an exported pack -- a MeshMC, MultiMC or
 * Prism zip, a Modrinth .mrpack, a CurseForge zip -- on this computer or at
 * a download link. A file dropped on the window, or handed over by
 * `meshmc --import`, lands here too (Main.qml's openImport()).
 *
 * The work is NewInstanceController.importFrom(), the classic window's
 * import task. Whatever it needs to ask on the way -- whether to update an
 * instance installed from the same pack, mods it may not download -- comes
 * through UiRequests like any other question. Import shows how it is going
 * in the footer, then opens the new instance; leaving the page does not
 * stop it.
 */
FormCard.FormCardPage {
    id: page

    required property var app
    readonly property var controller: page.app.shell && page.app.shell.newInstance ? page.app.shell.newInstance : null

    // The file or link to import, as typed, picked or dropped.
    property string source
    readonly property string trimmedSource: page.source.trim()
    readonly property bool sourceValid: page.trimmedSource.length > 0 && !!page.controller
                                        && page.controller.isImportSourceValid(page.trimmedSource)

    readonly property string installKey: "import/" + page.openedAt
    readonly property double openedAt: Date.now()
    property var watcher: null
    readonly property bool importing: !!page.watcher && page.watcher.running
    readonly property bool canImport: !page.importing && page.sourceValid && identity.name.length > 0

    // From the field, the file picker and a drop alike, so the three agree
    // and the name follows whichever changed last.
    function setSource(source) {
        page.source = source
        if (sourceField.text !== source)
            sourceField.text = source
        page.suggestName()
    }
    // The pack's file name, until a name is typed.
    function suggestName() {
        if (page.controller)
            identity.suggest(page.controller.suggestedNameForImportSource(page.trimmedSource))
    }

    function startImport() {
        if (!page.canImport)
            return
        var watcher = page.controller.importFrom(page.trimmedSource, identity.name, identity.group, identity.iconKey)
        if (!watcher) {
            page.app.notify(i18n("That file or link cannot be imported."))
            return
        }
        page.watcher = watcher
        page.app.trackModpackInstall(page.installKey, watcher, identity.name)
    }
    // For review snapshots (DevRoute.qml's "install").
    function installWhenReady() {
        if (!page.canImport)
            return page.importing
        page.startImport()
        return true
    }

    // How Navigator finds this page on top of the stack.
    objectName: "importInstancePage"
    title: i18nc("@title", "Import Instance")

    actions: [
        Kirigami.Action {
            text: page.importing ? i18nc("@action:button", "Importing…") : i18nc("@action:button", "Import")
            icon.name: "document-import"
            enabled: page.canImport
            onTriggered: page.startImport()
        }
    ]

    footer: ProgressFooter {
        active: page.importing
        status: page.watcher && page.watcher.status.length > 0 ? page.watcher.status
                                                              : i18nc("@info:status", "Importing…")
        progress: page.watcher ? page.watcher.progress : -1
    }

    Component.onCompleted: page.setSource(page.source)
    Connections {
        target: page.watcher
        function onFinished(ok) {
            if (!ok)
                return
            var id = page.watcher.instanceId
            if (!id || !page.app.openInstance(id)) {
                page.app.showSection("library")
                page.app.notify(i18n("“%1” is in your library.", page.watcher.title))
            }
        }
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 30
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Kirigami.Units.largeSpacing
        visible: !!page.watcher && page.watcher.failed
        type: Kirigami.MessageType.Error
        text: page.watcher && page.watcher.error.length > 0 ? page.watcher.error
                                                           : i18n("The instance could not be imported.")
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Pack")
    }
    FormCard.FormCard {
        enabled: !page.importing

        FormCard.FormTextFieldDelegate {
            id: sourceField
            label: i18nc("@label:textbox", "File or link")
            placeholderText: i18nc("@info:placeholder", "A .zip or .mrpack file, or a download link")
            status: Kirigami.MessageType.Error
            statusMessage: page.trimmedSource.length > 0 && !page.sourceValid
                           ? i18n("This doesn't look like a modpack file or link. Pick a .zip, .mrpack or .jar file, or paste a direct download link.")
                           : ""
            onTextEdited: page.setSource(text)
            onAccepted: page.startImport()
        }
        FormCard.FormDelegateSeparator {}
        FormCard.FormButtonDelegate {
            text: i18nc("@action:button", "Choose File…")
            icon.name: "document-open"
            onClicked: fileDialog.open()
        }
    }
    FormCard.FormSectionText {
        text: i18n("You can also drop a file or a link on this window. Packs exported from CurseForge, FTB, ATLauncher or Technic import the same way, from their .zip file.")
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Instance")
    }
    InstanceIdentityCard {
        id: identity
        Layout.bottomMargin: Kirigami.Units.largeSpacing * 2
        app: page.app
        enabled: !page.importing
        placeholderName: i18nc("@info:placeholder", "Named after the pack")
    }

    Dialogs.FileDialog {
        id: fileDialog
        title: i18nc("@title:dialog", "Import Instance")
        nameFilters: [i18n("Modpacks (*.zip *.mrpack)"), i18n("All files (*)")]
        onAccepted: page.setSource(selectedFile.toString())
    }
}
