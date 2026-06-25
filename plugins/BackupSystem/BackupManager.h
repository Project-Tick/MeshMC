/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * BackupManager — Core backup logic for the BackupSystem plugin.
 * Handles creating, restoring, exporting, and importing instance backups.
 */

#pragma once

#include "plugin/sdk/mmco_cxx_sdk.hpp"

/*
 * BackupManager talks to the host through the MMCO C ABI: every zip
 * operation is routed via the MMCOContext's zip_* function pointers
 * rather than #include "MMCZip.h". Callers hand the live context to
 * each instance.
 */

struct BackupEntry {
	QString name;		 // human-readable label
	QString fileName;	 // zip file name (e.g. "2026-01-15_14-30-00.zip")
	QString fullPath;	 // absolute path to the backup zip
	QDateTime timestamp; // when the backup was created
	qint64 sizeBytes;	 // file size
};

class BackupManager
{
  public:
	/* `ctx` is the MMCOContext owned by the plugin; the manager only
	 * uses it to drive zip_compress_dir / zip_extract.  May be null
	 * in unit tests — the zip-dependent methods will then no-op. */
	BackupManager(const QString& instanceId, const QString& instanceRoot,
				  MMCOContext* ctx = nullptr);

	QString backupDir() const;
	QList<BackupEntry> listBackups() const;
	BackupEntry createBackup(const QString& label = {});
	bool restoreBackup(const BackupEntry& entry);
	bool exportBackup(const BackupEntry& entry, const QString& destPath);
	BackupEntry importBackup(const QString& srcZipPath,
							 const QString& label = {});
	bool deleteBackup(const BackupEntry& entry);

  private:
	void ensureBackupDir();
	QString generateFileName(const QString& label) const;
	BackupEntry entryFromFile(const QString& filePath) const;

	/* Stage the instance root into a temp dir, skipping `.backups/`
	 * so we don't recurse into our own zip archive collection.
	 * Returns the temp dir path (caller deletes when done) or an
	 * empty string on failure. */
	QString stageInstance() const;

	QString m_instanceId;
	QString m_instanceRoot;
	QString m_backupDir;
	MMCOContext* m_ctx = nullptr;
};
