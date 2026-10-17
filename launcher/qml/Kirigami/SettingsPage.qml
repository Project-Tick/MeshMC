// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * One page of the settings window (SettingsView.qml): FormCards over the
 * launcher's settings, each saved the moment it changes -- there is no
 * Apply, as in every current KDE application's settings.
 */
FormCard.FormCardPage {
    id: page

    // The main window (Main.qml): its shell and navigate().
    required property var app
    readonly property var shell: page.app && page.app.shell ? page.app.shell : null

    // The launcher-wide settings every delegate on the page reads and writes.
    readonly property SettingsSource store: SettingsSource {
        adapter: page.shell && page.shell.settings ? page.shell.settings : null
    }

    // A passing message at the bottom of the settings window.
    function notify(message) {
        var window = page.QQC2.ApplicationWindow.window
        if (window && typeof window.showPassiveNotification === "function")
            window.showPassiveNotification(message)
    }

    // For review snapshots (DevRoute.qml's "scroll="): "bottom" or a pixel
    // offset.
    function scrollTo(where) {
        var flick = page.flickable
        if (!flick)
            return false
        var y = where === "bottom" ? flick.contentHeight : parseInt(where)
        flick.contentY = Math.max(0, Math.min(y, flick.contentHeight - flick.height))
        return true
    }
}
