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
 *
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QVector>

namespace ModPlatform
{

	struct SelectedMod {
		QString name;
		QString projectId; // platform-specific project ID
		QString versionId; // platform-specific version/file ID
		QString fileName;
		QString downloadUrl;
		QString sha1;
		int fileSize = 0;
		QString platform; // "curseforge" or "modrinth"
		QString mcVersion;
		QString loaders;
	};

	struct DependencyInfo {
		QString projectId;
		QString versionId;
		QString name;
		QString slug;
		QString fileName;
		QString downloadUrl;
		QString sha1;
		int fileSize = 0;
		QString platform;
		bool isRequired = true;
	};

	struct DownloadItem {
		QString name;
		QString fileName;
		QString downloadUrl;
		QString sha1;
		int fileSize = 0;
		bool isDependency = false;

		/* Provenance — required to write the sidecar after install and to
		 * power conflict / update detection. Fill these in when the item
		 * originates from a known platform; leave them empty for purely
		 * local installs. */
		QString platform; /* "modrinth" | "curseforge" | ""               */
		QString projectId;
		QString versionId;
		QString slug;

		/* When set, the downloader will replace any existing file with the
		 * same target name (typical for an "update" flow). When false, an
		 * existing file with matching SHA-1 is silently kept; mismatched
		 * but already-named files are an error. */
		bool replaceExisting = false;

		/* Used by the conflict analyzer to communicate which file on disk
		 * is being replaced. Optional; if non-empty the downloader will
		 * remove this file (and its sidecar) before writing the new one. */
		QString replacesFileName;
	};

	struct UnresolvedDep {
		QString name;
		QString projectId;
		QString platform;
	};

	inline QString curseForgeSha1FromFileObject(const QJsonObject& fileObj)
	{
		const auto hashes = fileObj.value("hashes").toArray();
		for (const auto& hashRaw : hashes) {
			const auto hashObj = hashRaw.toObject();
			if (hashObj.value("algo").toInt() == 1) {
				return hashObj.value("value").toString();
			}
		}
		return QString();
	}

} // namespace ModPlatform

Q_DECLARE_METATYPE(ModPlatform::SelectedMod)
Q_DECLARE_METATYPE(ModPlatform::DependencyInfo)
Q_DECLARE_METATYPE(ModPlatform::DownloadItem)
Q_DECLARE_METATYPE(ModPlatform::UnresolvedDep)
