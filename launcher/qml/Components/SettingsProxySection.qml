// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import MeshMC.Theme

/*
 * How MeshMC itself reaches the internet -- the QML equivalent of the
 * classic ProxyPage. Every field saves as it is set, same as the rest of
 * this window, so there is no separate Apply button; instead, whenever one
 * of the five proxy settings changes, applyProxySettings() below pushes the
 * whole configuration to the network layer immediately, the same effect
 * ProxyPage::apply() had when the classic dialog's OK button was pressed.
 * The group's description only ever says "Applied." after a change.
 */
SettingsScroll {
    id: root

    title: i18n("Proxy")
    description: i18n("How MeshMC reaches the internet. Only MeshMC's own downloads use it; Minecraft ignores proxy settings.")

    readonly property string proxyType: SettingsStore.string("ProxyType")
    readonly property bool needsAddress: root.proxyType === "SOCKS5" || root.proxyType === "HTTP"

    // A brief "Applied." confirmation after any push to the network layer,
    // since there is no Apply button here for it to have been the result of.
    property bool justApplied: false
    Timer {
        id: appliedTimer
        interval: 1600
        onTriggered: root.justApplied = false
    }

    Connections {
        target: SettingsStore.adapter
        function onValueChanged(id) {
            switch (id) {
                case "ProxyType":
                case "ProxyAddr":
                case "ProxyPort":
                case "ProxyUser":
                case "ProxyPass":
                    if (SettingsStore.adapter) {
                        SettingsStore.adapter.applyProxySettings(
                            SettingsStore.string("ProxyType"),
                            SettingsStore.string("ProxyAddr"),
                            SettingsStore.number("ProxyPort"),
                            SettingsStore.string("ProxyUser"),
                            SettingsStore.string("ProxyPass"))
                        root.justApplied = true
                        appliedTimer.restart()
                    }
                    break
            }
        }
    }

    SettingsGroup {
        width: parent.width
        title: i18n("Type")
        description: root.justApplied ? i18n("Applied.") : ""
        SettingChoice {
            key: "ProxyType"
            label: i18n("Proxy type")
            radio: true
            options: [
                { value: "None", label: i18n("None") },
                { value: "Default", label: i18n("System") },
                { value: "SOCKS5", label: i18n("SOCKS5") },
                { value: "HTTP", label: i18n("HTTP") }
            ]
        }
    }

    SettingsGroup {
        width: parent.width
        visible: root.needsAddress
        title: i18n("Address and port")
        SettingText {
            key: "ProxyAddr"
            label: i18n("Host")
            placeholder: "127.0.0.1"
        }
        SettingNumber {
            key: "ProxyPort"
            label: i18n("Port")
            from: 1
            to: 65535
        }
    }

    SettingsGroup {
        width: parent.width
        visible: root.needsAddress
        title: i18n("Authentication")
        description: i18n("Stored in plain text in MeshMC's configuration file.")
        SettingText {
            key: "ProxyUser"
            label: i18n("Username")
        }
        SettingText {
            key: "ProxyPass"
            label: i18n("Password")
            secret: true
        }
    }
}
