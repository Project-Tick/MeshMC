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
#include <QMetaType>
#include <QString>
#include <QVector>

namespace Flame
{

	struct ModpackAuthor {
		QString name;
		QString url;
	};

	struct IndexedVersion {
		int addonId;
		int fileId;
		QString version;
		QString mcVersion;
		QString downloadUrl;
		QString fileName;
	};

	struct IndexedPack {
		int addonId;
		QString name;
		QString description;
		QList<ModpackAuthor> authors;
		QString logoName;
		QString logoUrl;
		QString websiteUrl;

		bool versionsLoaded = false;
		QVector<IndexedVersion> versions;
	};

	void loadIndexedPack(IndexedPack& m, QJsonObject& obj);
	void loadIndexedPackVersions(IndexedPack& m, QJsonArray& arr);
} // namespace Flame

Q_DECLARE_METATYPE(Flame::IndexedPack)
