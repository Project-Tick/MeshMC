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

#include "ModInstallConflictAnalyzer.h"

#include "minecraft/mod/ModMetadataIndex.h"

const char* ModInstallConflictAnalyzer::statusLabel(Status s)
{
	switch (s) {
		case Status::Fresh:
			return "Fresh";
		case Status::AlreadyInstalled:
			return "Already installed";
		case Status::UpdateAvailable:
			return "Update available";
		case Status::NameConflict:
			return "Name conflict";
		case Status::FileNameClash:
			return "File name clash";
	}
	return "?";
}

QList<ModInstallConflictAnalyzer::Decision> ModInstallConflictAnalyzer::analyze(
	const QList<ModPlatform::DownloadItem>& items,
	std::shared_ptr<ModMetadataIndex> index)
{
	QList<Decision> out;
	out.reserve(items.size());

	for (const auto& item : items) {
		Decision d;
		d.item = item;

		if (!index) {
			d.status = Status::Fresh;
			out.append(d);
			continue;
		}

		// 1) Strongest signal: a sidecar with the same platform+projectId
		//    pins this exact mod, regardless of file name churn.
		ModMetadataIndex::Entry sameProject;
		if (!item.platform.isEmpty() && !item.projectId.isEmpty()) {
			sameProject =
				index->findByPlatformProject(item.platform, item.projectId);
		}
		if (sameProject.isValid()) {
			if (!item.versionId.isEmpty() &&
				sameProject.versionId == item.versionId) {
				d.status = Status::AlreadyInstalled;
				d.reason = QStringLiteral("Same version already installed (%1)")
							   .arg(sameProject.fileName);
			} else {
				d.status = Status::UpdateAvailable;
				d.reason = QStringLiteral("Will replace %1 (version %2 → %3)")
							   .arg(sameProject.fileName, sameProject.versionId,
									item.versionId);
				d.item.replaceExisting = true;
				d.item.replacesFileName = sameProject.fileName;
			}
			out.append(d);
			continue;
		}

		// 2) Same normalized name but a different platform / project.
		//    Could be the same mod via a different distribution channel
		//    (e.g. user previously installed the Modrinth build, now the
		//    CurseForge one is selected). Surface as a conflict so the
		//    caller can decide.
		const QString normalized = ModMetadataIndex::normalizeName(item.name);
		ModMetadataIndex::Entry sameName;
		if (!normalized.isEmpty()) {
			sameName = index->findByNormalizedName(normalized);
		}
		if (sameName.isValid() && !(sameName.platform == item.platform &&
									sameName.projectId == item.projectId)) {
			d.status = Status::NameConflict;
			d.reason = QStringLiteral("%1 already provides this mod (%2)")
						   .arg(sameName.platform.isEmpty()
									? QStringLiteral("Local copy")
									: sameName.platform,
								sameName.fileName);
			// Treat as a forced replace so the caller can opt in by
			// keeping the item in the plan.
			d.item.replaceExisting = true;
			d.item.replacesFileName = sameName.fileName;
			out.append(d);
			continue;
		}

		// 3) Same target file name, different origin. This is the case
		//    where two unrelated mods share a filename — keep the planned
		//    download but ask the downloader to overwrite.
		if (index->contains(item.fileName)) {
			d.status = Status::FileNameClash;
			d.reason = QStringLiteral("A different file named %1 is already "
									  "installed and will be replaced")
						   .arg(item.fileName);
			d.item.replaceExisting = true;
			d.item.replacesFileName = item.fileName;
			out.append(d);
			continue;
		}

		d.status = Status::Fresh;
		out.append(d);
	}
	return out;
}

QList<ModPlatform::DownloadItem>
ModInstallConflictAnalyzer::toDownloadPlan(const QList<Decision>& decisions)
{
	QList<ModPlatform::DownloadItem> out;
	out.reserve(decisions.size());
	for (const auto& d : decisions) {
		if (d.status == Status::AlreadyInstalled) {
			continue;
		}
		out.append(d.item);
	}
	return out;
}
