// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * The skin page's two grids: the user's own skins (the launcher's skins
 * folder), then the game's nine default skins, each shown with its own arm
 * width or the one picked for it. A click picks a skin for the preview and
 * "Wear"; a second click lets it go again.
 */
ColumnLayout {
    id: cards

    required property var editor

    // The menu for library skin @p name (rename, delete).
    signal menuRequested(string name, bool worn)

    Layout.fillWidth: true
    spacing: 0

    FormCard.FormHeader {
        title: i18nc("@title:group", "Your Skins")
    }
    FormCard.FormCard {
        FormCard.FormPlaceholderMessageDelegate {
            visible: cards.editor.skinList.length === 0
            text: i18n("No skins here yet")
            explanation: i18n("Add one from a file, a link or a player's name.")
        }
        FormCard.AbstractFormDelegate {
            visible: cards.editor.skinList.length > 0
            background: null
            focusPolicy: Qt.NoFocus
            contentItem: Flow {
                spacing: Kirigami.Units.smallSpacing
                Repeater {
                    model: cards.editor.skinList
                    delegate: SkinTile {
                        required property var modelData
                        name: modelData.name
                        spriteUrl: modelData.thumbnailUrl
                        worn: modelData.worn
                        hasMenu: true
                        selected: cards.editor.selectedDefault.length === 0 && cards.editor.selectedName === modelData.name
                        onClicked: cards.editor.selectLibrary(modelData.name)
                        onMenuRequested: cards.menuRequested(modelData.name, modelData.worn)
                    }
                }
            }
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Default Skins")
    }
    FormCard.FormCard {
        FormCard.FormTextDelegate {
            visible: cards.editor.defaultsLoading || cards.editor.defaultList.length === 0
            text: cards.editor.defaultsLoading ? i18n("Downloading the default skins…")
                                               : i18n("The default skins download when you are online.")
        }
        FormCard.AbstractFormDelegate {
            visible: cards.editor.defaultList.length > 0
            background: null
            focusPolicy: Qt.NoFocus
            contentItem: Flow {
                spacing: Kirigami.Units.smallSpacing
                Repeater {
                    model: cards.editor.defaultList
                    delegate: SkinTile {
                        id: character
                        required property var modelData
                        readonly property bool isSelected: cards.editor.selectedDefault === modelData.name
                        readonly property var shown: modelData.variants[(character.isSelected ? cards.editor.defaultSlim
                                                                                              : modelData.slim) ? 1 : 0]
                        name: modelData.name
                        spriteUrl: character.shown.thumbnailUrl || ""
                        ready: !!character.shown.ready
                        worn: modelData.variants[0].worn || modelData.variants[1].worn
                        selected: character.isSelected
                        onClicked: cards.editor.selectDefault(modelData)
                    }
                }
            }
        }
    }
}
