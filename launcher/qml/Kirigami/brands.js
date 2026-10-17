// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

.pragma library

/*
 * The mod loaders and modpack sites, each by its owner's own logo
 * (BrandIcon.qml draws one). Two kinds:
 *
 *  - bundled: the owner licenses the logo openly, so it ships in the
 *    resources (resources/meshmc/brands, a .license file beside each);
 *  - fetched: it does not, so BrandLogos downloads it from the owner's site
 *    on first use into the data folder's cache, and a Breeze icon stands in
 *    until then (and offline).
 *
 * Vanilla has no loader: the launcher's own grass block stands for it.
 */

var bundledRoot = "qrc:/icons/meshmc/brands/"
var bundled = {
    fabric: "fabric.svg",
    quilt: "quilt.svg",
    forge: "forge.svg",
    neoforge: "neoforge.svg",
    ftb: "ftb.svg",
    technic: "technic.svg"
}
var fetched = ["modrinth", "curseforge", "atlauncher"]
// Logos drawn as pixel art, to be scaled without smoothing.
var pixelArt = ["fabric", "neoforge"]

var vanilla = "vanilla"
var vanillaIcon = "image://instanceicon/grass"

// The brand id of a loader as InstanceList names it ("Fabric", "NeoForge",
// ...), as the providers name it ("neoforge"), or "" for vanilla.
function loaderId(name) {
    var id = String(name || "").toLowerCase().replace(/[^a-z]/g, "")
    return id.length > 0 ? id : vanilla
}

// What an Image can show for @p id right now, or "" for none yet. @p logos
// is the shell's BrandLogos, read with its `revision` so a binding through
// this asks again once a download lands.
function source(id, logos) {
    if (id === vanilla)
        return vanillaIcon
    if (bundled[id])
        return bundledRoot + bundled[id]
    if (fetched.indexOf(id) >= 0 && logos && typeof logos.url === "function") {
        logos.revision
        return logos.url(id)
    }
    return ""
}

function isPixelArt(id) {
    return id === vanilla || pixelArt.indexOf(id) >= 0
}
