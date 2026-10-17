// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.coreaddons as KCoreAddons
import org.kde.kirigami as Kirigami

import "brands.js" as Brands
import "format.js" as Format

/*
 * The top of a modpack's page (ProjectHeader): its logo, name and author,
 * how many have downloaded and follow it; beside them where it is from,
 * what the picked version runs on, when it was last updated and its
 * categories. `controls` -- Install and its choices -- go under the name.
 *
 * `pack` is the Discover result row; `version` the picked entry of the
 * pack's versions ({gameVersions, loaders, ...}), or null.
 */
ProjectHeader {
    id: header

    property var pack: ({})
    property string providerId
    property string providerName
    property var version: null
    // Loader ids as the projects spell their own names.
    property var loaderNames: ({})

    readonly property var updatedDate: Format.parseDate(header.pack.updated)
    readonly property string loader: header.version && (header.version.loaders || []).length > 0
                                     ? header.version.loaders[0] : ""
    readonly property string gameVersion: header.version && (header.version.gameVersions || []).length > 0
                                          ? header.version.gameVersions[0] : ""

    iconSource: header.pack.logoUrl || ""
    fallbackSource: "image://instanceicon/planks"
    title: header.pack.title || ""
    author: header.pack.author || ""

    stats: {
        var list = []
        if ((header.pack.downloads || 0) > 0)
            list.push({ iconName: "download-symbolic",
                        text: i18ncp("@info %2 is a short number such as 12.3K", "%2 download", "%2 downloads",
                                     header.pack.downloads, Format.compactNumber(header.pack.downloads)) })
        if ((header.pack.follows || 0) > 0)
            list.push({ iconName: "starred-symbolic",
                        text: i18ncp("@info %2 is a short number such as 12.3K", "%2 follower", "%2 followers",
                                     header.pack.follows, Format.compactNumber(header.pack.follows)) })
        return list
    }

    details: {
        var list = []
        if (header.providerName.length > 0)
            list.push({ label: i18nc("@label where a modpack is from", "Source:"),
                        text: header.providerName, brand: header.providerId })
        if (header.loader.length > 0 || header.gameVersion.length > 0) {
            var loaderName = header.loaderNames[header.loader]
                             || (header.loader.length > 0 ? header.loader.charAt(0).toUpperCase() + header.loader.slice(1) : "")
            list.push({ label: i18nc("@label what a modpack version runs on", "Runs on:"),
                        text: [loaderName, header.gameVersion].filter(s => s.length > 0).join(" "),
                        brand: Brands.loaderId(header.loader) })
        }
        if (header.updatedDate)
            list.push({ label: i18nc("@label", "Updated:"),
                        text: KCoreAddons.Format.formatRelativeDate(header.updatedDate, Locale.ShortFormat) })
        var categories = (header.pack.categories || []).map(c => c.length > 0 ? c.charAt(0).toUpperCase() + c.slice(1) : c)
        if (categories.length > 0)
            list.push({ label: i18nc("@label", "Categories:"),
                        text: categories.join(i18nc("@info separator between categories", ", ")) })
        return list
    }
}
