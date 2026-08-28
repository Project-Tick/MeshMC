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

#include <QString>
#include <QList>
#include <QDateTime>
#include <functional>

/*
 * BackupManager — instance backup snapshots (create, restore, export,
 * import, delete).
 *
 * Backups are plain zip archives stored in `<instanceRoot>/.backups/`.
 * The file name carries the creation timestamp and an optional label:
 *
 *   2026-01-15_14-30-00.zip
 *   2026-01-15_14-30-00_pre-launch.zip
 *
 * Keeping the metadata in the file name (rather than a side-car index)
 * means a backup directory copied between installs — or restored by
 * hand from a file manager — still lists correctly.
 *
 * This code used to live in the out-of-tree BackupSystem .mmco plugin
 * and drove zipping through the MMCO C ABI, which has no skip-predicate
 * for compression. The plugin worked around that by mirroring the whole
 * instance into a temp directory first — which is why its progress
 * started with a "Copying instance files..." phase. In core we call
 * MMCZip::compressDir() directly with an exclude filter, so that phase
 * and the disk-space spike that came with it are gone; only compression
 * is left to report on.
 *
 * Everything here blocks the calling thread. Callers that must not
 * block the GUI go through BackupTask, which runs createBackup() on a
 * worker thread and republishes this progress callback as Task
 * progress.
 */

struct BackupEntry {
	QString name;		  /* human-readable label, may be empty */
	QString fileName;	  /* zip file name */
	QString fullPath;	  /* absolute path to the backup zip */
	QDateTime timestamp;  /* when the backup was created */
	qint64 sizeBytes = 0; /* file size */

	bool isValid() const
	{
		return !fullPath.isEmpty();
	}
};

class BackupManager
{
  public:
	/* Reports how a long running backup is getting on:
	 *   status  — line describing the phase, empty to leave it alone.
	 *   details — second line, empty to leave it alone.
	 *   current/total — file counts; total 0 means "no idea yet".
	 * Called on whichever thread createBackup() runs on, so an
	 * implementation that touches the UI has to marshal (BackupTask
	 * does). Updates are already throttled to roughly one per percent —
	 * a callback per file would be one queued call per file for no
	 * visible gain. */
	using ProgressFn =
		std::function<void(const QString& status, const QString& details,
						   qint64 current, qint64 total)>;

	BackupManager(const QString& instanceId, const QString& instanceRoot);

	/* Absolute path of the per-instance backup directory. */
	QString backupDir() const;

	/* Newest first. */
	QList<BackupEntry> listBackups() const;

	/* Returns an invalid entry (empty fullPath) on failure. */
	BackupEntry createBackup(const QString& label = {},
							 const ProgressFn& progress = nullptr);

	/* Wipes the instance root (except .backups/) and unpacks `entry`
	 * over it. Destructive and not undoable — callers must confirm. */
	bool restoreBackup(const BackupEntry& entry);

	bool exportBackup(const BackupEntry& entry, const QString& destPath);
	BackupEntry importBackup(const QString& srcZipPath,
							 const QString& label = {});
	bool deleteBackup(const BackupEntry& entry);

  private:
	bool ensureBackupDir();
	QString generateFileName(const QString& label) const;
	BackupEntry entryFromFile(const QString& filePath) const;

	QString m_instanceId;
	QString m_instanceRoot;
	QString m_backupDir;
};
