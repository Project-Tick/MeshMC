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

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>

namespace ModPlatform
{

	struct SelectedMod {
		QString name;
		QString projectId; // platform-specific project ID
		QString versionId; // platform-specific version/file ID
		/* Platform slug. Recorded because it is the one name the two
		 * providers usually agree on, and because a packwiz sidecar is
		 * named after it. */
		QString slug;
		QString fileName;
		QString downloadUrl;
		QString sha1;
		int fileSize = 0;
		QString platform; // "curseforge" or "modrinth"
		QString mcVersion;
		QString loaders;
		/* "release" / "beta" / "alpha", or empty when the provider does
		 * not say. Carried through so the review dialog can show what
		 * kind of build is about to be installed. */
		QString versionType;
		/* The author forbade third-party downloads, so the API handed
		 * out no URL and the address being used points at the website
		 * instead. Worth knowing: it is the one kind of download that
		 * may need the user to fetch the file by hand. */
		bool browserDownloadOnly = false;
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
		QString versionType;
		bool isRequired = true;
		/* See SelectedMod::browserDownloadOnly. */
		bool browserDownloadOnly = false;

		/* Names of the things that asked for this, for the review
		 * dialog's "Required by" line. A list because two selected mods
		 * can need the same library, and the dependency is only
		 * downloaded once. */
		QStringList requiredBy;

		/* The same project is already on disk, at some other version.
		 * Not a reason to drop it - the file on disk may be for another
		 * Minecraft version, or damaged - but a reason to offer it
		 * unticked and let the user decide, which is what the reference
		 * launcher does with its `maybe_installed`. */
		bool maybeInstalled = false;
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

		/* The download address is the website's rather than the API's,
		 * because the author opted out of third-party downloads. Such a
		 * download can be refused, and the caller then has the option of
		 * asking the user to fetch the file by hand. */
		bool browserDownloadOnly = false;

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
