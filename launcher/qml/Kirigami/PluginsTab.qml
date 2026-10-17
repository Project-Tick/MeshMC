// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * What plugins show on this instance's page: each surface a plugin put at
 * the "instance page" anchor (shell.pluginSurfaces(1, instanceId)), one
 * after another under its title. The instance page offers this tab only
 * when there is a surface (`count`).
 */
FormTab {
    id: tab

    required property var app
    property var details: null
    // The shell's PluginSurfaceModel for this instance, or null.
    property var model: null
    readonly property alias count: surfaces.count
    readonly property alias firstTitle: surfaces.firstTitle

    PluginSurfaces {
        id: surfaces
        Layout.bottomMargin: Kirigami.Units.gridUnit
        model: tab.model
        // A single plugin page is already named by its tab.
        showTitles: surfaces.count > 1
    }
}
