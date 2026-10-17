// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

.pragma library

/*
 * The picture on an instance's card, and on a world without an icon of its
 * own: the instance's pack art when it is a platform pack (InstanceList's
 * packArt role: the pack's logo), else its newest screenshot (coverImage),
 * else one of the launcher's own rendered scenes, picked from the id so an
 * instance keeps its scene.
 *
 * The scenes are the custom interface's (MeshMC.Components' art/, rendered
 * by art/scenes/build.py) and are read from its resources by address; when
 * that module goes, art/ moves here with it.
 */

var artRoot = "qrc:/qt/qml/MeshMC/Components/art/"
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
    return artRoot + "scenes/card/" + (index < 10 ? "0" : "") + index + ".jpg"
}

// A square biome picture for a world with no icon.png, from @p seed (its
// folder name).
function worldUrl(seed) {
    return artRoot + "worlds/" + worldBiomes[hash(seed) % worldBiomes.length] + ".jpg"
}

// What an instance's card shows: {source, logo}. `logo` is true for pack
// art, a square logo to be shown whole rather than cropped like a picture.
// @p entry has InstanceList's roles.
function cover(entry) {
    var packArt = String(entry.packArt || "")
    if (packArt.length > 0)
        return { source: packArt, logo: true }
    var shot = String(entry.coverImage || "")
    if (shot.length > 0)
        return { source: shot, logo: false }
    return { source: sceneUrl(entry.instanceId), logo: false }
}
