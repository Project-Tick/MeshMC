// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as Dialogs
import org.kde.kirigami as Kirigami

/*
 * Adding skins to the library -- from a file, a link or another player's
 * name -- and a library skin's menu: rename, and delete (asked first; the
 * skin the account wears cannot be deleted). All through the account's
 * SkinsController (SkinEditorState.skins).
 */
Item {
    id: dialogs

    required property var editor
    readonly property var skins: dialogs.editor.skins

    function addFromFile() {
        fileDialog.open()
    }
    function addFromLink() {
        linkDialog.ask("")
    }
    function addFromPlayer() {
        playerDialog.ask("")
    }
    function openMenu(name, worn) {
        menu.target = name
        menu.targetWorn = worn
        menu.popup()
    }
    function added(ok) {
        if (ok)
            dialogs.editor.say(i18n("Skin added."), false)
    }

    Dialogs.FileDialog {
        id: fileDialog
        title: i18nc("@title:window", "Add a Skin")
        nameFilters: [i18n("PNG images (*.png)")]
        onAccepted: {
            var problem = dialogs.skins.importFile(selectedFile.toString())
            dialogs.editor.say(problem.length > 0 ? problem : i18n("Skin added."), problem.length > 0)
        }
    }

    TextPromptDialog {
        id: linkDialog
        title: i18nc("@title:dialog", "Add a Skin from a Link")
        subtitle: i18n("A direct link to the skin's PNG file.")
        placeholderText: "https://"
        confirmText: i18nc("@action:button", "Add")
        onSubmitted: (text) => {
            var w = dialogs.skins.importFromUrl(text)
            if (!w) {
                linkDialog.error = i18n("That is not a link. Links start with https://")
                return
            }
            linkDialog.close()
            dialogs.editor.run(w, i18n("The skin could not be added."), dialogs.added)
        }
    }

    TextPromptDialog {
        id: playerDialog
        title: i18nc("@title:dialog", "Add Another Player's Skin")
        subtitle: i18n("The Minecraft name of the player whose skin you want.")
        confirmText: i18nc("@action:button", "Add")
        onSubmitted: (text) => {
            var w = dialogs.skins.importFromUsername(text)
            if (!w) {
                playerDialog.error = i18n("Type a player's name.")
                return
            }
            playerDialog.close()
            dialogs.editor.run(w, i18n("The skin could not be added."), dialogs.added)
        }
    }

    TextPromptDialog {
        id: renameDialog
        property string target
        title: i18nc("@title:dialog", "Rename Skin")
        confirmText: i18nc("@action:button", "Rename")
        onSubmitted: (text) => {
            if (!dialogs.skins.rename(renameDialog.target, text)) {
                renameDialog.error = i18n("That name is empty or already taken.")
                return
            }
            if (dialogs.editor.selectedName === renameDialog.target)
                dialogs.editor.selectedName = text
            renameDialog.close()
        }
    }

    Kirigami.PromptDialog {
        id: deleteDialog
        property string target
        title: i18nc("@title:dialog", "Delete Skin")
        subtitle: i18n("Delete “%1” from your skins?", deleteDialog.target)
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Delete")
                icon.name: "edit-delete"
                onTriggered: {
                    deleteDialog.close()
                    var problem = dialogs.skins.remove(deleteDialog.target)
                    if (problem.length > 0)
                        dialogs.editor.say(problem, true)
                    else if (dialogs.editor.selectedName === deleteDialog.target)
                        dialogs.editor.selectedName = ""
                }
            }
        ]
    }

    QQC2.Menu {
        id: menu
        property string target
        property bool targetWorn: false
        QQC2.MenuItem {
            text: i18nc("@action:inmenu", "Rename…")
            icon.name: "edit-rename"
            onTriggered: {
                renameDialog.target = menu.target
                renameDialog.ask(menu.target)
            }
        }
        QQC2.MenuItem {
            text: i18nc("@action:inmenu", "Delete…")
            icon.name: "edit-delete"
            enabled: !menu.targetWorn
            onTriggered: {
                deleteDialog.target = menu.target
                deleteDialog.open()
            }
        }
    }
}
