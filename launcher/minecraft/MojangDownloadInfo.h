/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <QString>
#include <QMap>
#include <memory>

struct MojangDownloadInfo {
	// types
	typedef std::shared_ptr<MojangDownloadInfo> Ptr;

	// data
	/// Local filesystem path. WARNING: not used, only here so we can pass
	/// through mojang files unmolested!
	QString path;
	/// absolute URL of this file
	QString url;
	/// sha-1 checksum of the file
	QString sha1;
	/// size of the file in bytes
	int size;
};

struct MojangLibraryDownloadInfo {
	MojangLibraryDownloadInfo(MojangDownloadInfo::Ptr artifact)
		: artifact(artifact) {};
	MojangLibraryDownloadInfo() {};

	// types
	typedef std::shared_ptr<MojangLibraryDownloadInfo> Ptr;

	// methods
	MojangDownloadInfo* getDownloadInfo(QString classifier)
	{
		if (classifier.isNull()) {
			return artifact.get();
		}

		return classifiers[classifier].get();
	}

	// data
	MojangDownloadInfo::Ptr artifact;
	QMap<QString, MojangDownloadInfo::Ptr> classifiers;
};

struct MojangAssetIndexInfo : public MojangDownloadInfo {
	// types
	typedef std::shared_ptr<MojangAssetIndexInfo> Ptr;

	// methods
	MojangAssetIndexInfo() {}

	MojangAssetIndexInfo(QString id)
	{
		this->id = id;
		// HACK: ignore assets from other version files than Minecraft
		// workaround for stupid assets issue caused by amazon:
		// https://www.theregister.co.uk/2017/02/28/aws_is_awol_as_s3_goes_haywire/
		if (id == "legacy") {
			url = "https://launchermeta.mojang.com/mc/assets/legacy/"
				  "c0fd82e8ce9fbc93119e40d96d5a4e62cfa3f729/legacy.json";
		}
		// HACK
		else {
			url = "https://s3.amazonaws.com/Minecraft.Download/indexes/" + id +
				  ".json";
		}
		known = false;
	}

	// data
	int totalSize;
	QString id;
	bool known = true;
};
