// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * The Content tab's toolbar actions: Add Content (from Modrinth and
 * CurseForge) and Add Files first, Check for Updates where the kind has a
 * provider, then -- in the overflow menu -- Verify Dependencies for mods,
 * Export List and Open Folder. `contentTab` is the ContentTab, `contentFlows` its
 * ContentFlows.
 */
QtObject {
    id: owner

    required property var contentTab
    required property var contentFlows
    // Add Files, whose file dialog lives on the tab.
    signal addFilesRequested()

    property list<Kirigami.Action> actions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Add Content…")
            icon.name: "list-add"
            tooltip: i18n("Find mods, resource packs, shaders and data packs on Modrinth and CurseForge")
            enabled: !!owner.contentTab.details
            onTriggered: owner.contentTab.addContent()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Add Files…")
            icon.name: "document-import"
            enabled: owner.contentTab.unlocked
            onTriggered: owner.addFilesRequested()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Check for Updates")
            icon.name: "view-refresh"
            visible: owner.contentFlows.supportsUpdates()
            enabled: owner.contentTab.unlocked && owner.contentTab.totalCount > 0 && !owner.contentFlows.busy
            onTriggered: owner.contentFlows.checkUpdates()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Verify Dependencies")
            icon.name: "kt-check-data"
            visible: owner.contentTab.kind === "mods" && !!owner.contentFlows.updater
            enabled: owner.contentTab.unlocked && owner.contentTab.totalCount > 0 && !owner.contentFlows.busy
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: owner.contentFlows.verifyDependencies()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Export List…")
            icon.name: "document-export"
            enabled: owner.contentTab.totalCount > 0
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: owner.contentTab.exportList()
        },
        Kirigami.Action {
            text: i18nc("@action:button", "Open Folder")
            icon.name: "document-open-folder"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: owner.contentTab.app.call("openPath", owner.contentTab.content.dir(owner.contentTab.kind))
        }
    ]
}
