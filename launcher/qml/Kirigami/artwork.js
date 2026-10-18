// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

.pragma library

/*
 * The picture on an instance's card: the pack's cover photo when the
 * instance is a platform pack (InstanceList's packCover role: the picture
 * its catalogue leads with, cached on disk), else its newest screenshot
 * (coverImage), else the pack's logo (packArt), else the instance's own
 * icon. The launcher ships no pictures of its own: every one shown is the
 * pack's or the player's.
 */

// What an instance's card shows: {source, logo, icon}. `logo` is true for
// pack art and for the instance's icon, square pictures to be shown whole
// rather than cropped; the pack's cover is a photo, and cropped like one.
// The logo comes after the screenshot: a picture of the game fills the
// card, a logo only sits in the middle of it. `icon` is true for the
// instance's own icon, which a view that already shows that icon leaves
// out.
// @p entry has InstanceList's roles.
function cover(entry) {
    var photo = String(entry.packCover || "")
    if (photo.length > 0)
        return { source: photo, logo: false, icon: false }
    var shot = String(entry.coverImage || "")
    if (shot.length > 0)
        return { source: shot, logo: false, icon: false }
    var packArt = String(entry.packArt || "")
    if (packArt.length > 0)
        return { source: packArt, logo: true, icon: false }
    var key = String(entry.iconKey || "")
    return { source: "image://instanceicon/" + (key.length > 0 ? key : "default"), logo: true, icon: true }
}
