// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * Installed Plugins: every plugin MeshMC found, whether it loaded (or why
 * not), who made it, its signature, a link to its source, and a switch to
 * turn it on or off from the next start (PluginListModel). Only offered in
 * a build with the plugin host, where shell.pluginList() is not null.
 */
SettingsPage {
    id: page

    readonly property var plugins: page.shell && typeof page.shell.pluginList === "function"
                                   ? page.shell.pluginList() : null

    SeparatedCard {
        Layout.topMargin: Kirigami.Units.largeSpacing
        FormCard.FormPlaceholderMessageDelegate {
            visible: !page.plugins || page.plugins.count === 0
            text: i18n("No plugins are installed")
            explanation: i18n("Plugins add features to MeshMC.")
        }
        Repeater {
            model: page.plugins
            delegate: FormCard.FormTextDelegate {
                id: plugin
                required property int index
                // Through `model`: several roles (enabled, description...)
                // are properties of the delegate as well.
                required property var model

                text: plugin.model.version.length > 0
                      ? i18nc("@label plugin name and version", "%1 %2", plugin.model.name, plugin.model.version)
                      : plugin.model.name
                description: [plugin.model.description,
                              plugin.model.author.length > 0 ? i18nc("@info", "By %1", plugin.model.author) : "",
                              plugin.model.license.length > 0 ? i18nc("@info", "License: %1", plugin.model.license) : "",
                              plugin.model.dependencies.length > 0 ? i18nc("@info", "Needs %1", plugin.model.dependencies) : "",
                              i18nc("@info %1 load status, %2 signature", "%1 · %2", plugin.model.status, plugin.model.signature),
                              plugin.model.statusDetail].filter(s => s.length > 0).join("\n")
                trailing: RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    QQC2.ToolButton {
                        visible: plugin.model.sourceUrl.length > 0
                        icon.name: "internet-services"
                        text: i18nc("@action:button", "Source Code")
                        display: QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.text: plugin.model.sourceUrl
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        onClicked: Qt.openUrlExternally(plugin.model.sourceUrl)
                    }
                    QQC2.Switch {
                        checked: plugin.model.enabled
                        enabled: plugin.model.canToggle
                        Accessible.name: i18nc("@option:check", "Use %1", plugin.model.name)
                        onToggled: {
                            page.plugins.setEnabled(plugin.index, checked)
                            // Shows what the plugin list made of it, not the click.
                            checked = Qt.binding(() => plugin.model.enabled)
                        }
                    }
                }
            }
        }
    }
    FormCard.FormSectionText {
        text: page.plugins && page.plugins.restartNeeded
              ? i18n("Restart MeshMC to apply your changes.")
              : i18n("Turning a plugin on or off takes effect the next time MeshMC starts.")
    }
}
