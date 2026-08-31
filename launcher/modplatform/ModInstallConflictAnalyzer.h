/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Project Tick
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
