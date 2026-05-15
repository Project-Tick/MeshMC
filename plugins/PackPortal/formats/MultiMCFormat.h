/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MultiMC / MeshMC raw instance zip format
 *
 * Layout (rooted at <pack-name>/ inside the zip):
 *   instance.cfg
 *   mmc-pack.json   — the patch components MeshMC actually uses
 *   .minecraft/     — entire game directory copied verbatim
 *
 * This is the most-faithful round-trip — no external references, no
 * lossy translation. Mostly used to move an instance between machines.
 */

#pragma once

#include "../PackFormat.h"

namespace pack::multimc
{

/* The MultiMC export is a *directory tree* layered into the zip rather
 * than a separate JSON manifest, so the "manifest" the caller cares
 * about is just the synthesised instance.cfg / mmc-pack.json pair. */
QByteArray buildInstanceCfg(const ExportStage& stage);
QByteArray buildMmcPackJson(const ExportStage& stage);

/* Parse an extracted MultiMC pack tree. instance.cfg + mmc-pack.json
 * are read from `rootDir`. The overrides are .minecraft/ inside. */
bool parseTree(const QString& rootDir, PackInfo& info, QString* errorMsg = nullptr);

QString suggestedFileName(const PackInfo& info);

} // namespace pack::multimc
