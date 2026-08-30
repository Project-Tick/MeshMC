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

#include "ApplyStage.h"

#include "UpdateLock.h"
#include "UpdaterUtil.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

namespace
{

	//! The first stage exits right after starting us; it should be quick.
	constexpr int kPrepareExitTimeoutMs = 30 * 1000;

	//! How many previous versions to keep around.
	constexpr int kBackupsToKeep = 2;

	QString backupRoot(const UpdaterOptions& options)
	{
		return QDir(options.dataDir)
			.absoluteFilePath(QLatin1String(UpdaterUtil::kBackupDirName));
	}

} // namespace

ApplyStage::ApplyStage(const UpdaterOptions& options, UpdateLock& lock,
					   QObject* parent)
	: QObject(parent), m_options(options), m_lock(lock)
{
}

void ApplyStage::start()
{
	m_lock.setStage(QStringLiteral("apply"));

	if (m_options.waitPid > 0) {
		emit progress(QStringLiteral("Waiting for the first stage (pid %1)")
						  .arg(m_options.waitPid));
		if (!UpdaterUtil::waitForProcessExit(m_options.waitPid,
											 kPrepareExitTimeoutMs)) {
			emit finished(
				false,
				tr("The first stage of the update did not exit (pid %1). "
				   "Nothing has been changed.")
					.arg(m_options.waitPid));
			return;
		}
	}

	// It has just exited, so anything it had to rename out of the way earlier
	// can go now.
	const int swept = UpdaterUtil::sweepDisplacedFiles(m_options.root);
	if (swept > 0)
		emit progress(
			QStringLiteral("Removed %1 displaced file(s)").arg(swept));

	const QStringList incoming =
		UpdaterUtil::relativeFilePaths(m_options.source);
	if (incoming.isEmpty()) {
		emit finished(false,
					  tr("The unpacked update contains no files:\n%1")
						  .arg(QDir::toNativeSeparators(m_options.source)));
		return;
	}
	emit progress(QStringLiteral("Installing %1 file(s) from %2 into %3")
					  .arg(incoming.size())
					  .arg(m_options.source, m_options.root));

	QString error;
	if (!backup(incoming, error)) {
		emit finished(false, error);
		return;
	}

	if (!installFiles(incoming, error)) {
		// The lock stays: the installation may be a mix of two versions and
		// the next attempt should say so rather than pretend all is well.
		emit finished(false, error);
		return;
	}

	pruneBackups();

	// The archive was already removed after unpacking; this clears the
	// directory itself and anything an interrupted run left in it.
	UpdaterUtil::removeDirectoryTree(
		QDir(m_options.dataDir)
			.absoluteFilePath(QLatin1String(UpdaterUtil::kDownloadDirName)));

	m_lock.release();
	emit progress(QStringLiteral("Update complete."));

	relaunch();
	emit finished(true, QString());
}

bool ApplyStage::backup(const QStringList& incoming, QString& error)
{
	const QDir source(m_options.source);
	const QDir root(m_options.root);

	QStringList toBackUp;
	for (const QString& relative : incoming) {
		if (QFileInfo::exists(root.absoluteFilePath(relative)))
			toBackUp << relative;
	}

	if (toBackUp.isEmpty()) {
		emit progress(QStringLiteral(
			"Nothing to back up; the update only adds new files."));
		return true;
	}

	m_backupDir = QDir(backupRoot(m_options))
					  .absoluteFilePath(QDateTime::currentDateTime().toString(
						  QStringLiteral("yyyyMMdd-HHmmss")));
	if (!UpdaterUtil::ensureDirectory(m_backupDir)) {
		error = tr("Cannot create the backup directory:\n%1")
					.arg(QDir::toNativeSeparators(m_backupDir));
		return false;
	}

	emit progress(QStringLiteral("Backing up %1 file(s) to %2")
					  .arg(toBackUp.size())
					  .arg(m_backupDir));

	const QDir backupDir(m_backupDir);
	qint64 bytes = 0;
	for (const QString& relative : toBackUp) {
		const QString from = root.absoluteFilePath(relative);
		const QString to = backupDir.absoluteFilePath(relative);
		QString reason;
		if (!UpdaterUtil::installFile(from, to, reason)) {
			error = tr("Could not back up the current version before updating."
					   "\n\n%1\n\nNothing has been changed.")
						.arg(reason);
			return false;
		}
		bytes += QFileInfo(from).size();
	}

	emit progress(QStringLiteral("Backup complete, %1")
					  .arg(UpdaterUtil::formatBytes(bytes)));
	Q_UNUSED(source)
	return true;
}

bool ApplyStage::installFiles(const QStringList& incoming, QString& error)
{
	const QDir source(m_options.source);
	const QDir root(m_options.root);

	int installed = 0;
	for (const QString& relative : incoming) {
		const QString from = source.absoluteFilePath(relative);
		const QString to = root.absoluteFilePath(relative);

		QString reason;
		if (!UpdaterUtil::installFile(from, to, reason)) {
			error =
				tr("Could not install the update.\n\n%1\n\n%2 of %3 file(s) "
				   "had already been replaced. The previous version was saved "
				   "to:\n%4")
					.arg(reason)
					.arg(installed)
					.arg(incoming.size())
					.arg(m_backupDir.isEmpty()
							 ? tr("(no backup was made)")
							 : QDir::toNativeSeparators(m_backupDir));
			return false;
		}
		++installed;
	}

	emit progress(QStringLiteral("Installed %1 file(s)").arg(installed));
	return true;
}

void ApplyStage::pruneBackups()
{
	QDir root(backupRoot(m_options));
	if (!root.exists())
		return;

	// Names are timestamps, so lexical order is chronological order.
	QStringList backups =
		root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
	while (backups.size() > kBackupsToKeep) {
		const QString oldest = backups.takeFirst();
		if (UpdaterUtil::removeDirectoryTree(root.absoluteFilePath(oldest)))
			emit progress(QStringLiteral("Removed old backup %1").arg(oldest));
	}
}

void ApplyStage::relaunch()
{
	if (m_options.exec.isEmpty()) {
		emit progress(QStringLiteral("No --exec given, not restarting."));
		return;
	}
	if (!QFileInfo(m_options.exec).isFile()) {
		qWarning() << "ApplyStage: cannot restart, missing" << m_options.exec;
		return;
	}

	QProcess app;
#ifdef Q_OS_WIN
	// Without this Windows may decide that a program started by something
	// named "-updater" wants elevation, and prompt the user for no reason.
	QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
	environment.insert(QStringLiteral("__COMPAT_LAYER"),
					   QStringLiteral("RUNASINVOKER"));
	app.setProcessEnvironment(environment);
#endif
	app.setProgram(m_options.exec);
	app.setWorkingDirectory(m_options.root);

	if (!app.startDetached())
		qWarning() << "ApplyStage: could not restart" << m_options.exec << ":"
				   << app.errorString();
	else
		emit progress(QStringLiteral("Restarted %1").arg(m_options.exec));
}
