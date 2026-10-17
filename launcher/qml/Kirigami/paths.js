// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

.pragma library

/*
 * Between the file urls the system's file dialogs hand out and the plain
 * paths the settings keep.
 */

// A local path for @p url ("file:///a/b" -> "/a/b"); other text as it is.
function localPath(url) {
    var text = String(url || "")
    if (text.indexOf("file://") !== 0)
        return text
    return decodeURIComponent(text.slice("file://".length))
}

// A file url for an absolute @p path, for a dialog to start in; "" for a
// relative one (inside the data folder, which a dialog cannot see).
function folderUrl(path) {
    var text = String(path || "")
    return text.charAt(0) === "/" ? "file://" + encodeURI(text) : ""
}
