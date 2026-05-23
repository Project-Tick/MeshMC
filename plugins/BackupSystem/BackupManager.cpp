/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "BackupManager.h"

#include <QStandardPaths>
#include <QUuid>

BackupManager::BackupManager(const QString& instanceId,
							 const QString& instanceRoot, MMCOContext* ctx)
	: m_instanceId(instanceId), m_instanceRoot(instanceRoot), m_ctx(ctx)
{
	m_backupDir = QDir(m_instanceRoot).filePath(".backups");
	ensureBackupDir();
}

QString BackupManager::stageInstance() const
{
	/* Build a unique temp dir, then mirror everything from
	 * instanceRoot/ into it except for .backups/.  Hard-linking is
	 * tempting but cross-filesystem builds (e.g. instance on /mnt,
	 * temp on /tmp) break it; copy is portable and the cost is paid
	 * only at backup time. */
	const QString tempBase =
		QStandardPaths::writableLocation(QStandardPaths::TempLocation);
	const QString stage = QDir(tempBase).filePath(
		QStringLiteral("meshmc-backup-%1")
			.arg(QUuid::createUuid().toString(QUuid::Id128).left(12)));
	if (!QDir().mkpath(stage))
		return {};

	QDirIterator it(m_instanceRoot,
					QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden,
					QDirIterator::NoIteratorFlags);
	while (it.hasNext()) {
		it.next();
		const QFileInfo fi = it.fileInfo();
		if (fi.fileName() == QStringLiteral(".backups"))
			continue;

		auto copyTree = [](const QString& src, const QString& dst,
						   auto&& self) -> bool {
			QFileInfo fi(src);
			if (fi.isDir()) {
				if (!QDir().mkpath(dst))
					return false;
				QDirIterator inner(src, QDir::AllEntries |
											QDir::NoDotAndDotDot |
											QDir::Hidden);
				while (inner.hasNext()) {
					inner.next();
					const QString childSrc = inner.filePath();
					const QString childDst =
						QDir(dst).filePath(inner.fileName());
					if (!self(childSrc, childDst, self))
						return false;
				}
				return true;
			}
			return QFile::copy(src, dst);
		};

		const QString dst = QDir(stage).filePath(fi.fileName());
		if (!copyTree(fi.absoluteFilePath(), dst, copyTree)) {
			qWarning() << "[BackupSystem] stage copy failed for"
					   << fi.absoluteFilePath();
			QDir(stage).removeRecursively();
			return {};
		}
	}
	return stage;
}

QString BackupManager::backupDir() const
{
	return m_backupDir;
}

void BackupManager::ensureBackupDir()
{
	QDir().mkpath(m_backupDir);
}

QList<BackupEntry> BackupManager::listBackups() const
{
	QList<BackupEntry> result;

	QDir dir(m_backupDir);
	if (!dir.exists())
		return result;

	const auto files = dir.entryInfoList({"*.zip"}, QDir::Files, QDir::Time);
	for (const auto& fi : files) {
		result.append(entryFromFile(fi.absoluteFilePath()));
	}

	return result;
}

BackupEntry BackupManager::createBackup(const QString& label)
{
	ensureBackupDir();

	QString fileName = generateFileName(label);
	QString zipPath = QDir(m_backupDir).filePath(fileName);

	qDebug() << "[BackupSystem] Creating backup:" << zipPath << "from"
			 << m_instanceRoot;

	if (!m_ctx) {
		qWarning() << "[BackupSystem] No MMCO context available — "
					  "cannot compress.";
		return {};
	}

	/* Stage the instance into a temp dir (without .backups/) so the
	 * zip never contains nested backup archives. The host's
	 * zip_compress_dir takes a directory and walks it whole; we work
	 * around the missing skip-predicate by pre-filtering the tree. */
	const QString stage = stageInstance();
	if (stage.isEmpty()) {
		qWarning() << "[BackupSystem] Could not stage instance for backup.";
		return {};
	}

	const QByteArray zipUtf8 = zipPath.toUtf8();
	const QByteArray stageUtf8 = stage.toUtf8();
	const int rc = m_ctx->zip_compress_dir(
		m_ctx->module_handle, zipUtf8.constData(), stageUtf8.constData());
	QDir(stage).removeRecursively();

	if (rc != 0) {
		qWarning() << "[BackupSystem] Failed to create backup:" << zipPath;
		QFile::remove(zipPath);
		return {};
	}

	qDebug() << "[BackupSystem] Backup created successfully:" << zipPath;
	return entryFromFile(zipPath);
}

bool BackupManager::restoreBackup(const BackupEntry& entry)
{
	if (!QFile::exists(entry.fullPath)) {
		qWarning() << "[BackupSystem] Backup file not found:" << entry.fullPath;
		return false;
	}

	qDebug() << "[BackupSystem] Restoring backup:" << entry.fullPath << "to"
			 << m_instanceRoot;

	QDir instDir(m_instanceRoot);
	const auto entries = instDir.entryInfoList(
		QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
	for (const auto& fi : entries) {
		if (fi.fileName() == ".backups")
			continue;

		if (fi.isDir()) {
			QDir(fi.absoluteFilePath()).removeRecursively();
		} else {
			QFile::remove(fi.absoluteFilePath());
		}
	}

	if (!m_ctx) {
		qWarning() << "[BackupSystem] No MMCO context available — cannot "
					  "restore.";
		return false;
	}
	const QByteArray zipUtf8 = entry.fullPath.toUtf8();
	const QByteArray rootUtf8 = m_instanceRoot.toUtf8();
	const int rc = m_ctx->zip_extract(m_ctx->module_handle, zipUtf8.constData(),
									  rootUtf8.constData());
	if (rc != 0) {
		qWarning() << "[BackupSystem] Failed to extract backup:"
				   << entry.fullPath;
		return false;
	}

	qDebug() << "[BackupSystem] Restore complete.";
	return true;
}

bool BackupManager::exportBackup(const BackupEntry& entry,
								 const QString& destPath)
{
	if (!QFile::exists(entry.fullPath))
		return false;

	QDir().mkpath(QFileInfo(destPath).absolutePath());
	return QFile::copy(entry.fullPath, destPath);
}

BackupEntry BackupManager::importBackup(const QString& srcZipPath,
										const QString& label)
{
	if (!QFile::exists(srcZipPath))
		return {};

	ensureBackupDir();

	QString fileName = generateFileName(label);
	QString destPath = QDir(m_backupDir).filePath(fileName);

	if (!QFile::copy(srcZipPath, destPath))
		return {};

	qDebug() << "[BackupSystem] Imported backup:" << srcZipPath << "as"
			 << destPath;
	return entryFromFile(destPath);
}

bool BackupManager::deleteBackup(const BackupEntry& entry)
{
	return QFile::remove(entry.fullPath);
}

QString BackupManager::generateFileName(const QString& label) const
{
	QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
	if (label.isEmpty()) {
		return ts + ".zip";
	}
	QString safe = label;
	safe.replace(QRegularExpression("[^a-zA-Z0-9_-]"), "_");
	safe.truncate(64);
	return ts + "_" + safe + ".zip";
}

BackupEntry BackupManager::entryFromFile(const QString& filePath) const
{
	QFileInfo fi(filePath);
	BackupEntry entry;
	entry.fileName = fi.fileName();
	entry.fullPath = fi.absoluteFilePath();
	entry.sizeBytes = fi.size();
	entry.timestamp =
		fi.birthTime().isValid() ? fi.birthTime() : fi.lastModified();

	QString base = fi.completeBaseName();
	int thirdUs = base.indexOf('_', 20);
	if (thirdUs > 0 && thirdUs < base.size() - 1) {
		entry.name = base.mid(thirdUs + 1).replace('_', ' ');
	} else {
		entry.name = base;
	}

	return entry;
}
