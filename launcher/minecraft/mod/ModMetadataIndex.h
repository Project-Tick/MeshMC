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
 */

#pragma once

#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QString>
#include <QStringList>

/*
 * ModMetadataIndex
 *
 * Persistent sidecar index that records how each managed file in a mod-
 * like folder was installed (which remote platform, which project, which
 * version, the SHA-1 we recorded, whether it was pulled in as a transitive
 * dependency, when it was installed).
 *
 * The index lives in `<folder>/.index/`, and the sidecars are packwiz
 * `*.pw.toml` files - the format the rest of this ecosystem reads and
 * writes - so an instance's folder stays usable by packwiz itself and by
 * other launchers. See PackwizSidecar.h for the format. Sidecars written
 * by earlier versions of this launcher (`<filename>.json`) are still read,
 * and are replaced with a TOML one the next time the entry is written.
 *
 * Stale `*.json` entries (whose target file is gone) are pruned on
 * `load()`. Orphaned `*.pw.toml` files are left alone: in a packwiz pack
 * the sidecar is the definition and the file next to it may simply not
 * have been downloaded yet, so deleting it would throw away the pack.
 *
 * The class is thread-safe: all mutations are guarded by an internal mutex
 * so the model thread and the install / dependency-resolver tasks can hit
 * it concurrently.
 */
class ModMetadataIndex
{
  public:
	struct Entry {
		/* Identity */
		QString fileName;  /* Base file name as stored on disk, no .disabled */
		QString platform;  /* "modrinth" | "curseforge" | "local" | ""       */
		QString projectId; /* Platform-specific project ID                   */
		QString versionId; /* Platform-specific version / file ID            */

		/* Descriptive */
		QString name; /* Human-readable mod name at install time         */
		QString slug; /* Platform slug, if known                         */
		QString side; /* "client" | "server" | "both" | "" if unknown    */
		QString downloadUrl;

		/* The SHA-1 we recorded for the file, and the digest the sidecar
		 * carries verbatim.
		 *
		 * These are not the same thing. Sidecars name their digest with a
		 * `hash-format`, and other tools store whatever the provider gave
		 * them - Modrinth is asked for SHA-512 first - so `sha1` stays
		 * empty for a foreign entry whose format says something else,
		 * rather than handing a SHA-512 to code that will compare it
		 * against a SHA-1 and conclude every file is corrupt. */
		QString sha1;
		QString hashFormat;
		QString hash;

		qint64 fileSize = 0;

		/* Flags */
		bool isDependency = false; /* installed as a transitive dep?         */

		/* Bookkeeping */
		QDateTime installedAt;

		bool isValid() const
		{
			return !fileName.isEmpty();
		}
		bool hasPlatformOrigin() const
		{
			return !platform.isEmpty() && !projectId.isEmpty();
		}
	};

	explicit ModMetadataIndex(const QDir& folder);

	/* Returns the absolute path of the sidecar directory (`<folder>/.index`).
	 * Creates it lazily if it does not yet exist. */
	QString indexDir() const
	{
		return m_indexDir.absolutePath();
	}

	/* Re-read every sidecar file from disk. Stale entries (no underlying
	 * file in `<folder>`) are silently dropped. Safe to call repeatedly. */
	void load();

	/* In-memory accessors. All return-by-value to keep callers off the
	 * mutex. The string keys are file names WITHOUT any `.disabled` suffix. */
	Entry get(const QString& fileName) const;
	bool contains(const QString& fileName) const;
	QList<Entry> all() const;

	/* Lookup helpers used by conflict / update analyzers. */
	Entry findByPlatformProject(const QString& platform,
								const QString& projectId) const;
	Entry findByNormalizedName(const QString& normalizedName) const;
	Entry findBySha1(const QString& sha1) const;

	/* Write (or overwrite) the sidecar for `entry.fileName`. Also flushes
	 * the in-memory cache. Returns true on success. */
	bool put(const Entry& entry);

	/* Remove sidecar associated with `fileName`. Returns true if a sidecar
	 * existed and was deleted. */
	bool remove(const QString& fileName);

	/* Move sidecar to follow a renamed file. Used when a mod is toggled
	 * (`foo.jar` <-> `foo.jar.disabled`) or otherwise renamed in place. */
	void rename(const QString& oldFileName, const QString& newFileName);

	/* Normalize a human-readable mod name into a comparison key:
	 * lower-cased, parenthesized suffixes stripped, non-alphanumeric
	 * removed, whitespace collapsed. Mirrors DependencyResolver. */
	static QString normalizeName(const QString& name);

	/* Strip a trailing `.disabled` from a file name, if present. */
	static QString canonicalFileName(const QString& fileName);

  private:
	QString sidecarPath(const QString& sidecarName) const;

	/* Delete `sidecarName` only when it really describes `canonicalName`.
	 *
	 * A sidecar is named after the project, not after the file, so two
	 * versions of one mod sitting in the folder together aim at the same
	 * name. Removing one of them must not take the other's provenance
	 * with it, so the file is read back before it is deleted. */
	bool dropSidecarOwnedBy(const QString& sidecarName,
							const QString& canonicalName) const;

	/* Legacy sidecars from before this folder spoke packwiz. Read-only:
	 * an entry that gets written again lands in a `.pw.toml` and the old
	 * file is removed, so a folder converts itself as it is used. */
	static Entry parseJson(const QByteArray& bytes);

	QDir m_folder;	 /* the mods/resourcepacks/... folder */
	QDir m_indexDir; /* <folder>/.index */
	mutable QMutex m_mutex;
	QHash<QString, Entry> m_entries; /* canonical fileName -> entry */

	/* Canonical fileName -> the sidecar file it was read from or written
	 * to. Needed because the sidecar's name comes from the slug, which
	 * cannot be recovered from the file name it describes. */
	QHash<QString, QString> m_sidecars;
};
