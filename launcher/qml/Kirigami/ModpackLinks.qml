// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * Where a modpack lives besides here: the links its provider gives
 * (Modrinth: its page, source, issues, wiki, Discord and donations, see
 * ModrinthModpackDetail.links), else its page on the provider, from its
 * slug. Each opens in the browser.
 */
ColumnLayout {
    id: section

    // The detail's links, [{kind, url, platform}], when it has any.
    property var detailLinks: []
    property string provider
    property string providerName
    property string slug

    readonly property var links: {
        var list = section.detailLinks ? section.detailLinks.slice() : []
        if (!list.some(l => l.kind === "page")) {
            var url = section.pageUrl(section.provider, section.slug || "")
            if (url.length > 0)
                list.unshift({ kind: "page", url: url })
        }
        return list
    }
    // The pack's own page on its provider, or null.
    readonly property var pageLink: section.links.find(l => l.kind === "page") || null

    function pageUrl(provider, slug) {
        if (slug.length === 0)
            return ""
        if (provider === "modrinth")
            return "https://modrinth.com/modpack/" + slug
        if (provider === "curseforge")
            return "https://www.curseforge.com/minecraft/modpacks/" + slug
        if (provider === "technic")
            return "https://www.technicpack.net/modpack/" + slug
        return ""
    }
    function linkText(link) {
        switch (link.kind) {
        case "page":
            return i18nc("@action:button %1 is a site such as Modrinth", "View on %1", section.providerName)
        case "source":
            return i18nc("@action:button", "Source Code")
        case "issues":
            return i18nc("@action:button", "Report a Problem")
        case "wiki":
            return i18nc("@action:button", "Wiki")
        case "discord":
            return i18nc("@action:button", "Discord")
        case "donation":
            // Modrinth names an unlisted site "Other".
            return link.platform && link.platform.length > 0 && link.platform !== "Other"
                   ? i18nc("@action:button %1 is a site such as Patreon", "Donate on %1", link.platform)
                   : i18nc("@action:button", "Donate")
        }
        return link.url
    }
    function linkIcon(kind) {
        return ({ page: "globe", source: "code-context", issues: "tools-report-bug", wiki: "help-contents",
                  discord: "dialog-messages", donation: "donate" })[kind] || "link"
    }

    visible: section.links.length > 0
    spacing: Kirigami.Units.largeSpacing

    Kirigami.Heading {
        level: 2
        text: i18nc("@title", "Links")
    }
    Flow {
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing
        Repeater {
            model: section.links
            QQC2.Button {
                required property var modelData
                text: section.linkText(modelData)
                icon.name: section.linkIcon(modelData.kind)
                flat: true
                onClicked: Qt.openUrlExternally(modelData.url)
                QQC2.ToolTip.text: modelData.url
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }
    }
}
