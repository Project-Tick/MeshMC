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

#include <QString>
#include <QVector>
#include <QUrl>

namespace Flame
{
	struct File {
		// NOTE: throws JSONValidationError
		bool parseFromBytes(const QByteArray& bytes);

		int projectId = 0;
		int fileId = 0;
		// NOTE: the opposite to 'optional'. This is at the time of writing
		// unused.
		bool required = true;

		// our
		bool resolved = false;
		QString fileName;
		QUrl url;

		/* SHA-1 digest and length of the file, as the API reports them.
		 *
		 * Recorded so an update can tell whether the file it is about to
		 * fetch is already sitting in the instance: two versions of a
		 * pack are mostly the same files, and the only trustworthy
		 * answer to "do I already have this one" is what is on disk.
		 *
		 * Left empty/zero when the response does not carry them, which
		 * costs nothing but a download. */
		QString sha1;
		qint64 fileSize = 0;

		QString targetFolder = QLatin1String("mods");
		enum class Type {
			Unknown,
			Folder,
			Ctoc,
			SingleFile,
			Cmod2,
			Modpack,
			Mod
		} type = Type::Mod;
	};

	struct Modloader {
		QString id;
		bool primary = false;
	};

	struct Minecraft {
		QString version;
		QString libraries;
		QVector<Flame::Modloader> modLoaders;
	};

	struct Manifest {
		QString manifestType;
		int manifestVersion = 0;
		Flame::Minecraft minecraft;
		QString name;
		QString version;
		QString author;
		QVector<Flame::File> files;
		QString overrides;
	};

	void loadManifest(Flame::Manifest& m, const QString& filepath);
} // namespace Flame
