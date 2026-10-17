// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.kirigamiaddons.settings as KirigamiSettings

/*
 * The launcher's settings, the way current KDE applications show theirs:
 * KirigamiAddons' ConfigurationView, a window of its own beside the main
 * one on the desktop (its pages listed down the side) and pages pushed in
 * the main window on a phone. One module per page; a module whose C++ side
 * this build lacks (the plugin host, the updater, the launcher's logs)
 * stays out of the list.
 *
 * open(moduleId) shows it on that page; Main.qml's navigate("settings/<id>")
 * comes here. Ids: general, java, minecraft, console, appearance, commands,
 * plugins, pluginsettings, proxy, tools, logupload, logs, updates, about,
 * aboutkde.
 */
KirigamiSettings.ConfigurationView {
    id: view

    required property var app
    readonly property var shell: view.app.shell

    window: view.app

    // The window (or the phone's page) while open, else null.
    readonly property var settingsWindow: view.configViewItem

    // Shows the settings on @p moduleId (the first page for "" or one this
    // build does not offer). Already open, they go to that page.
    function show(moduleId) {
        var id = view.moduleIds().indexOf(moduleId) >= 0 ? moduleId : ""
        // The open window keeps its page; a different one means a fresh
        // window on it.
        if (view.configViewItem && id.length > 0 && view.currentModule() !== id)
            view.configViewItem.close()
        if (!view.configViewItem && view.opensAsSheet && view.openAsWindow(id))
            return true
        view.open(id)
        return true
    }

    // ConfigurationView makes its desktop window window-modal over the main
    // one, and macOS presents a window-modal child as a sheet: no title
    // bar, so no close button (only Escape closed it). There it is built
    // the same way but application-modal, a window of its own.
    readonly property bool opensAsSheet: Qt.platform.os === "osx" && !Kirigami.Settings.isMobile
    function openAsWindow(moduleId) {
        const component = Qt.createComponent("org.kde.kirigamiaddons.settings.private", "ConfigWindow")
        if (component.status !== Component.Ready)
            return false
        view.configViewItem = component.createObject(view.app, {
            defaultModule: moduleId,
            modules: view.modules,
            width: Kirigami.Units.gridUnit * 50,
            height: Kirigami.Units.gridUnit * 30,
            minimumWidth: Kirigami.Units.gridUnit * 50,
            minimumHeight: Kirigami.Units.gridUnit * 30,
            title: view.title,
            modality: Qt.ApplicationModal,
        })
        view.configViewItem.closing.connect(event => {
            if (event.accepted) {
                view.configViewItem.destroy()
                view.configViewItem = null
            }
        })
        return true
    }
    function moduleIds() {
        var ids = []
        for (var i = 0; i < view.modules.length; ++i) {
            if (view.modules[i].visible)
                ids.push(view.modules[i].moduleId)
        }
        return ids
    }
    // The id of the page on show, or "".
    function currentModule() {
        var stack = view.configViewItem && view.configViewItem.pageStack ? view.configViewItem.pageStack : null
        var page = stack ? stack.currentItem : null
        return page && page.moduleId ? page.moduleId : ""
    }

    // What plugins add to the settings (PluginSurfaces.qml), counted so
    // their page is only listed while there is something on it.
    readonly property var pluginSurfaces: view.shell && typeof view.shell.pluginSurfaces === "function"
                                          ? view.shell.pluginSurfaces(0, "") : null
    readonly property Instantiator surfaceCounter: Instantiator {
        model: view.pluginSurfaces
        delegate: QtObject {}
    }

    component Module: KirigamiSettings.ConfigurationModule {
        required property Component component
        page: () => component
    }

    modules: [
        Module {
            moduleId: "general"
            text: i18nc("@action:button settings page", "General")
            icon.name: "configure-symbolic"
            component: Component { SettingsGeneralPage { app: view.app; readonly property string moduleId: "general" } }
        },
        Module {
            moduleId: "java"
            text: i18nc("@action:button settings page", "Java & Memory")
            icon.name: "application-x-java"
            component: Component { SettingsJavaPage { app: view.app; readonly property string moduleId: "java" } }
        },
        Module {
            moduleId: "minecraft"
            text: i18nc("@action:button settings page", "Minecraft")
            icon.name: "applications-games-symbolic"
            component: Component { SettingsMinecraftPage { app: view.app; readonly property string moduleId: "minecraft" } }
        },
        Module {
            moduleId: "console"
            text: i18nc("@action:button settings page", "Console")
            icon.name: "utilities-terminal-symbolic"
            component: Component { SettingsConsolePage { app: view.app; readonly property string moduleId: "console" } }
        },
        Module {
            moduleId: "appearance"
            text: i18nc("@action:button settings page", "Appearance")
            icon.name: "preferences-desktop-theme-global-symbolic"
            component: Component { SettingsAppearancePage { app: view.app; readonly property string moduleId: "appearance" } }
        },
        Module {
            moduleId: "commands"
            text: i18nc("@action:button settings page", "Custom Commands")
            icon.name: "system-run-symbolic"
            component: Component { SettingsCommandsPage { app: view.app; readonly property string moduleId: "commands" } }
        },
        Module {
            moduleId: "plugins"
            text: i18nc("@action:button settings page", "Installed Plugins")
            icon.name: "plugins-symbolic"
            visible: !!view.shell && typeof view.shell.pluginList === "function" && !!view.shell.pluginList()
            component: Component { SettingsPluginsPage { app: view.app; readonly property string moduleId: "plugins" } }
        },
        Module {
            moduleId: "pluginsettings"
            text: i18nc("@action:button settings page", "Plugin Settings")
            icon.name: "preferences-plugin-symbolic"
            visible: view.surfaceCounter.count > 0
            component: Component {
                FormCard.FormCardPage {
                    readonly property string moduleId: "pluginsettings"
                    PluginSurfaces {
                        model: view.pluginSurfaces
                    }
                }
            }
        },
        Module {
            moduleId: "proxy"
            text: i18nc("@action:button settings page", "Proxy")
            icon.name: "preferences-system-network-symbolic"
            component: Component { SettingsProxyPage { app: view.app; readonly property string moduleId: "proxy" } }
        },
        Module {
            moduleId: "tools"
            text: i18nc("@action:button settings page", "External Tools")
            icon.name: "tools-symbolic"
            component: Component { SettingsToolsPage { app: view.app; readonly property string moduleId: "tools" } }
        },
        Module {
            moduleId: "logupload"
            text: i18nc("@action:button settings page", "Log Upload")
            icon.name: "cloud-upload-symbolic"
            component: Component { SettingsLogUploadPage { app: view.app; readonly property string moduleId: "logupload" } }
        },
        Module {
            moduleId: "logs"
            text: i18nc("@action:button settings page", "Launcher Logs")
            icon.name: "view-list-text-symbolic"
            visible: !!view.shell && !!view.shell.launcherLogs
            component: Component { SettingsLauncherLogsPage { app: view.app; readonly property string moduleId: "logs" } }
        },
        Module {
            moduleId: "updates"
            text: i18nc("@action:button settings page", "Updates")
            icon.name: "update-none-symbolic"
            visible: !!view.shell && !!view.shell.updateSettings && view.shell.updateSettings.available
            component: Component { SettingsUpdatesPage { app: view.app; readonly property string moduleId: "updates" } }
        },
        Module {
            moduleId: "about"
            text: i18nc("@action:button settings page", "About MeshMC")
            icon.name: "help-about-symbolic"
            category: i18nc("@title:group", "About")
            component: Component { AboutMeshMCPage { app: view.app; readonly property string moduleId: "about" } }
        },
        Module {
            moduleId: "aboutkde"
            text: i18nc("@action:button settings page", "About KDE")
            icon.name: "kde-symbolic"
            category: i18nc("@title:group", "About")
            component: Component { FormCard.AboutKDEPage { readonly property string moduleId: "aboutkde" } }
        }
    ]
}
