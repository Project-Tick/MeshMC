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

namespace Modrinth
{

	struct IndexedVersion {
		QString id;
		QString projectId;
		QString name;
		QString versionNumber;
		QString mcVersion;
		QString downloadUrl;
		int downloadSize = 0;
		QString sha1;
		QString loaders;
	};

	struct IndexedPack {
		QString projectId;
		QString slug;
		QString name;
		QString description;
		QString author;
		QString iconUrl;
		int downloads = 0;

		bool versionsLoaded = false;
		QVector<IndexedVersion> versions;
	};

	void loadIndexedPack(IndexedPack& pack, QJsonObject& obj);
	void loadIndexedPackVersions(IndexedPack& pack, QJsonArray& arr);

} // namespace Modrinth

Q_DECLARE_METATYPE(Modrinth::IndexedPack)
