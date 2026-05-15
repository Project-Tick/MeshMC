/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Modrinth .mrpack format
 *
 * Spec: https://docs.modrinth.com/docs/modpacks/format_definition/
 *
 * Layout:
 *   modrinth.index.json   — manifest
 *   overrides/            — files copied verbatim into the instance
 *   client-overrides/     — client-only overrides
 *   server-overrides/     — server-only overrides
 */

#pragma once

#include "../PackFormat.h"

namespace pack::mrpack
{

/* Build the modrinth.index.json string from the staged export. */
QByteArray buildManifest(const ExportStage& stage);

/* Parse a manifest file. Returns true if it is a recognised mrpack
 * manifest, regardless of whether all referenced files are reachable. */
bool parseManifest(const QByteArray& json, PackInfo& info,
				   QList<PackFile>& outFiles, QString* errorMsg = nullptr);

/* The recommended file name for an exported pack. */
QString suggestedFileName(const PackInfo& info);

} // namespace pack::mrpack
