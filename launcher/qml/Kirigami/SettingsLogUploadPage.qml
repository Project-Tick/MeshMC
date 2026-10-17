// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * Log Upload: the paste.ee key uploaded logs go out with -- MeshMC's shared
 * one (the literal "meshmc"), or one of the user's own. The key itself is
 * never shown again once set.
 *
 * `ownKey` is the page's own state, set from the setting once: following
 * the setting on every read would snap the radio back to the shared key
 * the moment "My own key" is picked, before a key has been typed.
 */
SettingsPage {
    id: page

    readonly property string sharedKey: "meshmc"
    property bool ownKey: false
    Component.onCompleted: page.ownKey = page.store.string("PasteEEAPIKey") !== page.sharedKey

    FormCard.FormHeader {
        title: i18nc("@title:group", "Upload With")
    }
    FormCard.FormCard {
        FormCard.FormRadioDelegate {
            text: i18nc("@option:radio", "MeshMC's shared key")
            description: i18n("Uploads of up to 12 MB.")
            checked: !page.ownKey
            onToggled: if (checked) {
                page.ownKey = false
                page.store.setValue("PasteEEAPIKey", page.sharedKey)
            }
        }
        FormCard.FormRadioDelegate {
            text: i18nc("@option:radio", "My own key")
            description: i18n("Pairs uploads with your paste.ee account.")
            checked: page.ownKey
            onToggled: if (checked) page.ownKey = true
        }
        FormCard.FormDelegateSeparator {
            visible: page.ownKey
        }
        FormCard.FormPasswordFieldDelegate {
            visible: page.ownKey
            label: i18nc("@label:textbox", "API key")
            placeholderText: i18nc("@info:placeholder", "Paste your key here")
            onEditingFinished: if (text.trim().length > 0) {
                page.store.setValue("PasteEEAPIKey", text.trim())
                text = ""
                page.notify(i18n("The key is saved."))
            }
        }
        FormCard.FormLinkDelegate {
            visible: page.ownKey
            text: i18nc("@action:button", "Get a key at paste.ee")
            url: "https://paste.ee/account/api"
        }
    }
}
