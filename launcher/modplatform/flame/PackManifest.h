/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
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
