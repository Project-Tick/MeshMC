/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 */

#pragma once

#include <QList>
#include <QString>
#include <memory>

#include "modplatform/ModDownloadTypes.h"

class ModMetadataIndex;

/*
 * ModInstallConflictAnalyzer
 *
 * Pure-function helper that classifies a set of `DownloadItem`s against
 * the currently installed mods (as represented by `ModMetadataIndex`).
 * Used by ModFolderPage and the update flow to decide what to actually
 * write to disk.
 */
class ModInstallConflictAnalyzer
{
  public:
	enum class Status {
		Fresh,			  /* Nothing comparable on disk; install normally */
		AlreadyInstalled, /* Same platform+projectId AND versionId present */
		UpdateAvailable,  /* Same platform+projectId, different versionId */
		NameConflict,	  /* Same normalized name from a different origin */
		FileNameClash	  /* Same target file name from a different source */
	};

	struct Decision {
		ModPlatform::DownloadItem item; /* Item, possibly with replaceExisting /
										   replacesFileName set */
		Status status = Status::Fresh;
		QString reason; /* Human-readable detail, surfaced in the UI */
	};

	/* Classify each input item. The returned list has the same length and
	 * ordering as `items`. `index` may be null, in which case every item
	 * is treated as Fresh. */
	static QList<Decision>
	analyze(const QList<ModPlatform::DownloadItem>& items,
			std::shared_ptr<ModMetadataIndex> index);

	/* Filter a decision list down to what should actually be downloaded:
	 *   - Fresh, NameConflict, FileNameClash: kept as-is.
	 *   - UpdateAvailable: kept with `replaceExisting=true` and
	 *     `replacesFileName` pointing at the old file.
	 *   - AlreadyInstalled: dropped.
	 * The caller is responsible for asking the user about NameConflict /
	 * FileNameClash through whatever dialog is appropriate. */
	static QList<ModPlatform::DownloadItem>
	toDownloadPlan(const QList<Decision>& decisions);

	static const char* statusLabel(Status s);
};
