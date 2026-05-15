/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * CurseForge modpack format (a.k.a. Twitch / Flame .zip)
 *
 * Reference: launcher/modplatform/flame/PackManifest.h
 *
 * Layout:
 *   manifest.json    — modloader list + references to CurseForge mods
 *   modlist.html     — pretty-printed mod list, optional
 *   overrides/       — files copied verbatim into the instance
 *
 * Constraints: every referenced mod MUST resolve to a CurseForge
 * project + file id. Files with no curseProjectId/curseFileId pair
 * fall back to being embedded under overrides/.
 */

#pragma once

#include "../PackFormat.h"

namespace pack::curseforge
{

/* Emit manifest.json + modlist.html. Returns the manifest bytes;
 * modlist HTML returned through the out-parameter. */
QByteArray buildManifest(const ExportStage& stage, QByteArray* modlistHtml);

/* Parse manifest.json. Files with project+file id stay external,
 * everything else is reported with `embed=true` so the caller knows
 * to look under overrides/. */
bool parseManifest(const QByteArray& json, PackInfo& info,
				   QList<PackFile>& outFiles, QString* errorMsg = nullptr);

QString suggestedFileName(const PackInfo& info);

} // namespace pack::curseforge
