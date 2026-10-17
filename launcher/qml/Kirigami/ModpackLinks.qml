// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * Where a modpack lives besides here: the links its provider gives
 * (Modrinth: its page, source, issues, wiki, Discord and donations, see
 * ModrinthModpackDetail.links), else its page on the provider, from its
 * slug. Each is a link with its icon, one to a line, as KDE's software
 * centre lists an application's; the page's own link has the site's logo.
 */
ColumnLayout {
    id: section

    // The detail's links, [{kind, url, platform}], when it has any.
    property var detailLinks: []
    property string provider
    property string providerName
    property string slug
    // The shell's BrandLogos, for the site's logo.
    property var logos: null

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
        return ({ page: "globe-symbolic", source: "code-context-symbolic", issues: "tools-report-bug-symbolic",
                  wiki: "documentation-symbolic", discord: "dialog-messages-symbolic",
                  donation: "help-donate-symbolic" })[kind] || "link-symbolic"
    }

    visible: section.links.length > 0
    spacing: Kirigami.Units.smallSpacing

    Kirigami.Heading {
        level: 2
        type: Kirigami.Heading.Type.Primary
        text: i18nc("@title", "Links")
    }
    Repeater {
        model: section.links
        RowLayout {
            id: link
            required property var modelData
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            BrandIcon {
                visible: link.modelData.kind === "page"
                brand: section.provider
                logos: section.logos
                fallback: section.linkIcon("page")
            }
            Kirigami.Icon {
                visible: link.modelData.kind !== "page"
                implicitWidth: Kirigami.Units.iconSizes.small
                implicitHeight: Kirigami.Units.iconSizes.small
                source: section.linkIcon(link.modelData.kind)
            }
            Kirigami.UrlButton {
                Layout.fillWidth: true
                text: section.linkText(link.modelData)
                url: link.modelData.url
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
            }
        }
    }
}
