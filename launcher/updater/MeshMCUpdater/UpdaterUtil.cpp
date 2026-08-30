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

#include "UpdaterUtil.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#endif

namespace UpdaterUtil
{

	namespace
	{

		constexpr int kInstallAttempts = 4;
		constexpr int kInstallRetryMs = 250;

	} // namespace

	bool isProcessRunning(qint64 pid)
	{
		if (pid <= 0)
			return false;

#ifdef Q_OS_WIN
		const HANDLE handle =
			OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
		if (handle == nullptr)
			return false; // gone, or not ours to look at -- treat as gone
		const DWORD state = WaitForSingleObject(handle, 0);
		CloseHandle(handle);
		return state == WAIT_TIMEOUT;
#else
		// Signal 0 performs the permission and existence checks without
		// delivering anything. EPERM means it exists but belongs to someone
		// else.
		if (::kill(static_cast<pid_t>(pid), 0) == 0)
			return true;
		return errno == EPERM;
#endif
	}

	bool waitForProcessExit(qint64 pid, int timeoutMs)
	{
		if (pid <= 0)
			return true;

#ifdef Q_OS_WIN
		const HANDLE handle =
			OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
		if (handle == nullptr)
			return true; // already gone
		const DWORD state =
			WaitForSingleObject(handle, static_cast<DWORD>(qMax(0, timeoutMs)));
		CloseHandle(handle);
		return state == WAIT_OBJECT_0;
#else
		QElapsedTimer timer;
		timer.start();
		while (isProcessRunning(pid)) {
			if (timer.hasExpired(timeoutMs))
				return false;
			QThread::msleep(50);
		}
		return true;
#endif
	}

	bool ensureDirectory(const QString& path)
	{
		if (path.isEmpty())
			return false;
		if (QFileInfo(path).isDir())
			return true;
		return QDir().mkpath(path);
	}

	bool installFile(const QString& from, const QString& to, QString& error)
	{
		if (!ensureDirectory(QFileInfo(to).absolutePath())) {
			error = QStringLiteral("cannot create directory %1")
						.arg(QFileInfo(to).absolutePath());
			return false;
		}

		QString lastFailure;
		for (int attempt = 1; attempt <= kInstallAttempts; ++attempt) {
			if (QFileInfo::exists(to) && !QFile::remove(to)) {
				// A mapped image cannot be deleted, but it can be renamed, and
				// the name is then immediately free for the replacement. The
				// leftover is swept up before the next update.
				const QString displaced = to + QLatin1String(kDisplacedSuffix);
				QFile::remove(displaced);
				if (!QFile::rename(to, displaced)) {
					lastFailure =
						QStringLiteral("%1 is in use and could not be replaced")
							.arg(to);
					QThread::msleep(kInstallRetryMs);
					continue;
				}
				qWarning() << "installFile: displaced in-use file" << to << "->"
						   << displaced;
			}

			QFile source(from);
			if (source.copy(to))
				return true;

			lastFailure = source.errorString();
			QThread::msleep(kInstallRetryMs);
		}

		error = QStringLiteral("%1 -> %2: %3").arg(from, to, lastFailure);
		return false;
	}

	QStringList relativeFilePaths(const QString& dir)
	{
		QStringList out;
		const QDir base(dir);
		if (!base.exists())
			return out;

		QDirIterator it(base.absolutePath(),
						QDir::Files | QDir::Hidden | QDir::System |
							QDir::NoDotAndDotDot,
						QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			out << base.relativeFilePath(it.fileInfo().absoluteFilePath());
		}
		out.sort();
		return out;
	}

	bool removeDirectoryTree(const QString& path)
	{
		if (path.isEmpty() || !QFileInfo(path).isDir())
			return true;
		return QDir(path).removeRecursively();
	}

	int sweepDisplacedFiles(const QString& root)
	{
		int removed = 0;
		QDirIterator it(
			root,
			QStringList{QStringLiteral("*") + QLatin1String(kDisplacedSuffix)},
			QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
			QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			if (QFile::remove(it.fileInfo().absoluteFilePath()))
				++removed;
		}
		return removed;
	}

	QString formatBytes(qint64 bytes)
	{
		static const char* units[] = {"B", "KiB", "MiB", "GiB"};
		double value = static_cast<double>(bytes);
		int unit = 0;
		while (value >= 1024.0 && unit < 3) {
			value /= 1024.0;
			++unit;
		}
		return unit == 0 ? QStringLiteral("%1 B").arg(bytes)
						 : QStringLiteral("%1 %2")
							   .arg(value, 0, 'f', 1)
							   .arg(QLatin1String(units[unit]));
	}

} // namespace UpdaterUtil
