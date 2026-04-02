/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "DesktopServices.h"
#include <QDir>
#include <QDesktopServices>
#include <QProcess>
#include <QDebug>

/**
 * This shouldn't exist, but until QTBUG-9328 and other unreported bugs are
 * fixed, it needs to be a thing.
 */
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

template <typename T>
bool IndirectOpen(T callable, qint64* pid_forked = nullptr)
{
	auto pid = fork();
	if (pid_forked) {
		if (pid > 0)
			*pid_forked = pid;
		else
			*pid_forked = 0;
	}
	if (pid == -1) {
		qWarning() << "IndirectOpen failed to fork: " << errno;
		return false;
	}
	// child - do the stuff
	if (pid == 0) {
		// unset all this garbage so it doesn't get passed to the child process
		qunsetenv("LD_PRELOAD");
		qunsetenv("LD_LIBRARY_PATH");
		qunsetenv("LD_DEBUG");
		qunsetenv("QT_PLUGIN_PATH");
		qunsetenv("QT_FONTPATH");

		// open the URL
		auto status = callable();

		// detach from the parent process group.
		setsid();

		// die. now. do not clean up anything, it would just hang forever.
		_exit(status ? 0 : 1);
	} else {
		// parent - assume it worked.
		int status;
		while (waitpid(pid, &status, 0)) {
			if (WIFEXITED(status)) {
				return WEXITSTATUS(status) == 0;
			}
			if (WIFSIGNALED(status)) {
				return false;
			}
		}
		return true;
	}
}
#endif

namespace DesktopServices
{
	bool openDirectory(const QString& path, bool ensureExists)
	{
		qDebug() << "Opening directory" << path;
		QDir parentPath;
		QDir dir(path);
		if (!dir.exists()) {
			parentPath.mkpath(dir.absolutePath());
		}
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
		return QProcess::startDetached("xdg-open", QStringList()
													   << dir.absolutePath());
#else
		return QDesktopServices::openUrl(
			QUrl::fromLocalFile(dir.absolutePath()));
#endif
	}

	bool openFile(const QString& path)
	{
		qDebug() << "Opening file" << path;
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
		return QProcess::startDetached("xdg-open", QStringList() << path);
#else
		return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
#endif
	}

	bool openFile(const QString& application, const QString& path,
				  const QString& workingDirectory, qint64* pid)
	{
		qDebug() << "Opening file" << path << "using" << application;
		return QProcess::startDetached(application, QStringList() << path,
									   workingDirectory, pid);
	}

	bool run(const QString& application, const QStringList& args,
			 const QString& workingDirectory, qint64* pid)
	{
		qDebug() << "Running" << application << "with args" << args.join(' ');
		return QProcess::startDetached(application, args, workingDirectory,
									   pid);
	}

	bool openUrl(const QUrl& url)
	{
		qDebug() << "Opening URL" << url.toString();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
		QStringList args;
		args << url.toString();
		return QProcess::startDetached("xdg-open", args);
#else
		return QDesktopServices::openUrl(url);
#endif
	}

} // namespace DesktopServices
