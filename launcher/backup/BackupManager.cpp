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

#include "backup/BackupManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QRegularExpression>
#include <QDebug>
#include <memory>

#include "FileSystem.h"
#include "MMCZip.h"

namespace
{
	/* Timestamp prefix every generated backup file name carries. */
	QString timestampFormat()
	{
		return QStringLiteral("yyyy-MM-dd_HH-mm-ss");
	}

	/* Name of the archive directory inside the instance root. */
	const QLatin1String kBackupDirName(".backups");

	/* True for paths that must never end up inside a backup. Paths
	 * arrive relative to the instance root with '/' separators, as
	 * handed out by MMCZip::compressDir(). */
	bool isExcludedFromBackup(const QString& relPath)
	{
		return relPath == kBackupDirName ||
			   relPath.startsWith(QLatin1String(".backups/"));
	}
} // namespace

BackupManager::BackupManager(const QString& instanceId,
							 const QString& instanceRoot)
	: m_instanceId(instanceId), m_instanceRoot(instanceRoot)
{
	m_backupDir = QDir(m_instanceRoot).filePath(kBackupDirName);
}

QString BackupManager::backupDir() const
{
	return m_backupDir;
}

bool BackupManager::ensureBackupDir()
{
	if (!FS::ensureFolderPathExists(m_backupDir)) {
		qWarning() << "[Backup] Could not create backup directory"
				   << m_backupDir;
		return false;
	}
	return true;
}

QList<BackupEntry> BackupManager::listBackups() const
{
	QList<BackupEntry> result;

	QDir dir(m_backupDir);
	if (!dir.exists())
		return result;

	const auto files =
		dir.entryInfoList({QStringLiteral("*.zip")}, QDir::Files, QDir::Time);
	for (const auto& fi : files) {
		result.append(entryFromFile(fi.absoluteFilePath()));
	}

	return result;
}

BackupEntry BackupManager::createBackup(const QString& label,
										const ProgressFn& progress)
{
	if (!ensureBackupDir())
		return {};

	const QString fileName = generateFileName(label);
	const QString zipPath = QDir(m_backupDir).filePath(fileName);

	qDebug() << "[Backup] Creating backup of instance" << m_instanceId << "->"
			 << zipPath;

	MMCZip::ProgressFunction zipProgress;
	if (progress) {
		progress(QObject::tr("Compressing backup..."), QString(), 0, 0);

		/* compressDir() calls back once per file. Forwarding every one
		 * of them would be a queued call per file for no visible gain,
		 * so only speak up when the percentage actually moves. */
		auto lastReported = std::make_shared<qint64>(-1);
		zipProgress = [progress, lastReported](qint64 current, qint64 total) {
			const qint64 step = qMax(qint64(1), total / 100);
			if (current != total && current / step == *lastReported)
				return;
			*lastReported = current / step;
			progress(QString(),
					 QObject::tr("%1 / %2 files").arg(current).arg(total),
					 current, total);
		};
	}

	if (!MMCZip::compressDir(zipPath, m_instanceRoot, &isExcludedFromBackup,
							 zipProgress)) {
		qWarning() << "[Backup] Failed to create backup:" << zipPath;
		// A half-written zip is worse than no zip: it would show up in
		// the list as a restorable snapshot.
		QFile::remove(zipPath);
		return {};
	}

	qDebug() << "[Backup] Backup created:" << zipPath;
	return entryFromFile(zipPath);
}

bool BackupManager::restoreBackup(const BackupEntry& entry)
{
	if (!QFile::exists(entry.fullPath)) {
		qWarning() << "[Backup] Backup file not found:" << entry.fullPath;
		return false;
	}

	qDebug() << "[Backup] Restoring" << entry.fullPath << "into"
			 << m_instanceRoot;

	// Clear the instance root, keeping the archive directory itself —
	// otherwise restoring would delete every other snapshot, including
	// the one being restored.
	QDir instDir(m_instanceRoot);
	const auto entries = instDir.entryInfoList(
		QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
	for (const auto& fi : entries) {
		if (fi.fileName() == kBackupDirName)
			continue;
		if (!FS::deletePath(fi.absoluteFilePath())) {
			qWarning() << "[Backup] Could not remove" << fi.absoluteFilePath()
					   << "- aborting restore to avoid a half-wiped instance";
			return false;
		}
	}

	if (!MMCZip::extractDir(entry.fullPath, m_instanceRoot).has_value()) {
		qWarning() << "[Backup] Failed to extract backup:" << entry.fullPath;
		return false;
	}

	qDebug() << "[Backup] Restore complete.";
	return true;
}

bool BackupManager::exportBackup(const BackupEntry& entry,
								 const QString& destPath)
{
	if (!QFile::exists(entry.fullPath))
		return false;

	if (!FS::ensureFilePathExists(destPath))
		return false;

	return QFile::copy(entry.fullPath, destPath);
}

BackupEntry BackupManager::importBackup(const QString& srcZipPath,
										const QString& label)
{
	if (!QFile::exists(srcZipPath))
		return {};

	if (!ensureBackupDir())
		return {};

	const QString destPath =
		QDir(m_backupDir).filePath(generateFileName(label));

	if (!QFile::copy(srcZipPath, destPath)) {
		qWarning() << "[Backup] Could not copy" << srcZipPath << "to"
				   << destPath;
		return {};
	}

	qDebug() << "[Backup] Imported" << srcZipPath << "as" << destPath;
	return entryFromFile(destPath);
}

bool BackupManager::deleteBackup(const BackupEntry& entry)
{
	return QFile::remove(entry.fullPath);
}

QString BackupManager::generateFileName(const QString& label) const
{
	const QString ts =
		QDateTime::currentDateTime().toString(timestampFormat());
	if (label.isEmpty())
		return ts + QStringLiteral(".zip");

	// Labels end up in a file name, so keep them boring. The same
	// substitution runs when the label is read back in entryFromFile().
	static const QRegularExpression unsafeChars(
		QStringLiteral("[^a-zA-Z0-9_-]"));
	QString safe = label;
	safe.replace(unsafeChars, QStringLiteral("_"));
	safe.truncate(64);
	return ts + QLatin1Char('_') + safe + QStringLiteral(".zip");
}

BackupEntry BackupManager::entryFromFile(const QString& filePath) const
{
	const QFileInfo fi(filePath);

	BackupEntry entry;
	entry.fileName = fi.fileName();
	entry.fullPath = fi.absoluteFilePath();
	entry.sizeBytes = fi.size();

	// The generated name is the authoritative record of when a backup was
	// taken: birth time is not available on every filesystem, and both
	// birth and modification time are rewritten by ordinary file copies
	// (which is exactly what export/import does).
	const QString base = fi.completeBaseName();
	static const QRegularExpression nameRe(QStringLiteral(
		"^(\\d{4}-\\d{2}-\\d{2}_\\d{2}-\\d{2}-\\d{2})(?:_(.+))?$"));
	const auto match = nameRe.match(base);
	if (match.hasMatch()) {
		entry.timestamp =
			QDateTime::fromString(match.captured(1), timestampFormat());
		entry.name =
			match.captured(2).replace(QLatin1Char('_'), QLatin1Char(' '));
	}

	if (!entry.timestamp.isValid()) {
		entry.timestamp =
			fi.birthTime().isValid() ? fi.birthTime() : fi.lastModified();
	}
	if (entry.name.isEmpty() && !match.hasMatch()) {
		// Hand-dropped zip that does not follow our naming scheme.
		entry.name = base;
	}

	return entry;
}
