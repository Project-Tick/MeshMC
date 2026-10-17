// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

.pragma library

/*
 * The picture on an instance's card, and on a world without an icon of its
 * own: the pack's cover photo when the instance is a platform pack
 * (InstanceList's packCover role: the picture its catalogue leads with,
 * cached on disk), else its newest screenshot (coverImage), else the
 * pack's logo (packArt), else one of the launcher's own rendered scenes,
 * picked from the id so an instance keeps its scene.
 *
 * The pictures are this module's own resources (art/, see CMakeLists.txt).
 */

var artRoot = "qrc:/qt/qml/MeshMC/Kirigami/art/"
var sceneCount = 32
var worldBiomes = ["plains", "desert", "snow", "jungle", "ocean", "cave", "nether", "end"]

// A small, stable string hash: the same id always picks the same scene.
function hash(text) {
    var h = 5381
    var s = String(text || "mesh")
    for (var i = 0; i < s.length; ++i)
        h = ((h * 33) ^ s.charCodeAt(i)) >>> 0
    return h
}

// One of the 16:9 scenes, for @p seed (an instance id).
function sceneUrl(seed) {
    var index = hash(seed) % sceneCount
    return artRoot + "scenes/" + (index < 10 ? "0" : "") + index + ".jpg"
}

// A square biome picture for a world with no icon.png, from @p seed (its
// folder name).
function worldUrl(seed) {
    return artRoot + "worlds/" + worldBiomes[hash(seed) % worldBiomes.length] + ".jpg"
}

// What an instance's card shows: {source, logo}. `logo` is true for pack
// art, a square logo to be shown whole rather than cropped like a picture;
// the pack's cover is a photo, and cropped like one. The logo comes after
// the screenshot: a picture of the game fills the card, a logo only sits
// in the middle of it.
// @p entry has InstanceList's roles.
function cover(entry) {
    var photo = String(entry.packCover || "")
    if (photo.length > 0)
        return { source: photo, logo: false }
    var shot = String(entry.coverImage || "")
    if (shot.length > 0)
        return { source: shot, logo: false }
    var packArt = String(entry.packArt || "")
    if (packArt.length > 0)
        return { source: packArt, logo: true }
    return { source: sceneUrl(entry.instanceId), logo: false }
}
