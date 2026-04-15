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
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2015-2021 MultiMC Contributors
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

#include <QJsonObject>
#include <memory>

#include "Exception.h"
#include "meta/BaseEntity.h"
#include <set>

namespace Meta
{
	class Index;
	class Version;
	class VersionList;

	enum class MetadataVersion { Invalid = -1, InitialRelease = 1 };

	class ParseException : public Exception
	{
	  public:
		using Exception::Exception;
	};
	struct Require {
		bool operator==(const Require& rhs) const
		{
			return uid == rhs.uid;
		}
		bool operator<(const Require& rhs) const
		{
			return uid < rhs.uid;
		}
		bool deepEquals(const Require& rhs) const
		{
			return uid == rhs.uid && equalsVersion == rhs.equalsVersion &&
				   suggests == rhs.suggests;
		}
		QString uid;
		QString equalsVersion;
		QString suggests;
	};

	inline Q_DECL_PURE_FUNCTION uint qHash(const Require& key,
										   uint seed = 0) Q_DECL_NOTHROW
	{
		return qHash(key.uid, seed);
	}

	using RequireSet = std::set<Require>;

	void parseIndex(const QJsonObject& obj, Index* ptr);
	void parseVersion(const QJsonObject& obj, Version* ptr);
	void parseVersionList(const QJsonObject& obj, VersionList* ptr);

	MetadataVersion parseFormatVersion(const QJsonObject& obj,
									   bool required = true);
	void serializeFormatVersion(QJsonObject& obj, MetadataVersion version);

	// FIXME: this has a different shape than the others...FIX IT!?
	void parseRequires(const QJsonObject& obj, RequireSet* ptr,
					   const char* keyName = "requires");
	void serializeRequires(QJsonObject& objOut, RequireSet* ptr,
						   const char* keyName = "requires");
	MetadataVersion currentFormatVersion();
} // namespace Meta

Q_DECLARE_METATYPE(std::set<Meta::Require>)
