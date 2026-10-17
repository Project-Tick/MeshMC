// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * Proxy: how MeshMC itself reaches the internet. Every field saves as it is
 * set, and whenever one of the five proxy settings changes the whole
 * configuration goes to the network layer at once (QmlShell::
 * applyProxySettings()), so there is no Apply either.
 */
SettingsPage {
    id: page

    readonly property string proxyType: page.store.string("ProxyType")
    readonly property bool needsAddress: page.proxyType === "SOCKS5" || page.proxyType === "HTTP"
    readonly property var proxyKeys: ["ProxyType", "ProxyAddr", "ProxyPort", "ProxyUser", "ProxyPass"]

    Connections {
        target: page.store
        function onChanged(key) {
            if (page.proxyKeys.indexOf(key) < 0)
                return
            page.app.call("applyProxySettings")
            applied.restart()
        }
    }
    // Several fields change together (a type, then its address): one
    // message for them.
    Timer {
        id: applied
        interval: 300
        onTriggered: page.notify(i18n("Proxy settings applied."))
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Proxy Type")
    }
    FormCard.FormCard {
        Repeater {
            model: [
                { value: "None", label: i18nc("@option:radio proxy", "None"),
                  description: i18n("Connect directly.") },
                { value: "Default", label: i18nc("@option:radio proxy", "System"),
                  description: i18n("Use this computer's proxy settings.") },
                { value: "SOCKS5", label: i18nc("@option:radio proxy", "SOCKS5"), description: "" },
                { value: "HTTP", label: i18nc("@option:radio proxy", "HTTP"), description: "" }
            ]
            delegate: FormCard.FormRadioDelegate {
                id: choice
                required property var modelData
                readonly property bool current: page.proxyType === choice.modelData.value
                text: choice.modelData.label
                description: choice.modelData.description
                // Checked follows the setting, not the click: a click only
                // asks for the change, and the binding is put back.
                autoExclusive: false
                checked: choice.current
                onClicked: {
                    if (!choice.current)
                        page.store.setValue("ProxyType", choice.modelData.value)
                    checked = Qt.binding(() => choice.current)
                }
            }
        }
    }
    FormCard.FormSectionText {
        text: i18n("Only MeshMC's own downloads use the proxy; Minecraft ignores it.")
    }

    FormCard.FormHeader {
        visible: page.needsAddress
        title: i18nc("@title:group", "Address")
    }
    FormCard.FormCard {
        visible: page.needsAddress
        SettingText {
            source: page.store
            key: "ProxyAddr"
            label: i18nc("@label:textbox", "Host")
            placeholderText: "127.0.0.1"
        }
        FormCard.FormDelegateSeparator {}
        SettingNumber {
            source: page.store
            key: "ProxyPort"
            label: i18nc("@label:spinbox", "Port")
            from: 1
            to: 65535
            // A port is a plain number, never "8,080".
            textFromValue: (value, locale) => String(value)
        }
    }

    FormCard.FormHeader {
        visible: page.needsAddress
        title: i18nc("@title:group", "Authentication")
    }
    FormCard.FormCard {
        visible: page.needsAddress
        SettingText {
            source: page.store
            key: "ProxyUser"
            label: i18nc("@label:textbox", "Username")
        }
        FormCard.FormDelegateSeparator {}
        FormCard.FormPasswordFieldDelegate {
            id: password
            label: i18nc("@label:textbox", "Password")
            readonly property string stored: page.store.string("ProxyPass")
            onStoredChanged: if (!fieldActiveFocus) text = stored
            Component.onCompleted: text = stored
            onEditingFinished: if (text !== stored) page.store.setValue("ProxyPass", text)
        }
    }
    FormCard.FormSectionText {
        visible: page.needsAddress
        text: i18n("The username and password are kept in plain text in MeshMC's configuration file.")
    }
}
