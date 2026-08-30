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

#include "UpdaterLog.h"

#include "UpdaterUtil.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>

#include <cstdio>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace UpdaterLog
{

	namespace
	{

		QMutex g_mutex;
		QFile* g_file = nullptr;
		QtMessageHandler g_previousHandler = nullptr;
		bool g_mirrorToConsole = false;
		QString g_path;

		const char* levelTag(QtMsgType type)
		{
			switch (type) {
				case QtDebugMsg:
					return "D";
				case QtInfoMsg:
					return "I";
				case QtWarningMsg:
					return "W";
				case QtCriticalMsg:
					return "C";
				case QtFatalMsg:
					return "F";
			}
			return "?";
		}

		void handler(QtMsgType type, const QMessageLogContext& context,
					 const QString& message)
		{
			// The message handler is called from whatever thread emitted the
			// message; QFile is not thread safe.
			QMutexLocker locker(&g_mutex);

			const QString line =
				QStringLiteral("%1 [%2] %3 | %4\n")
					.arg(QDateTime::currentDateTime().toString(
							 Qt::ISODateWithMs),
						 QString::number(QCoreApplication::applicationPid()),
						 QLatin1String(levelTag(type)), message);

			if (g_file != nullptr) {
				g_file->write(line.toUtf8());
				// Flushed per line on purpose: the runs worth reading are the
				// ones that end in a crash or a kill, and a buffered tail helps
				// nobody.
				g_file->flush();
			}

			if (g_mirrorToConsole) {
				fputs(qPrintable(line), stderr);
				fflush(stderr);
			}

			if (type == QtFatalMsg && g_previousHandler != nullptr)
				g_previousHandler(type, context, message);
		}

		//! Pick a log directory, preferring \a dataDir but never giving up.
		QString resolveLogDirectory(const QString& dataDir)
		{
			if (!dataDir.isEmpty()) {
				const QString logs = QDir(dataDir).absoluteFilePath("logs");
				if (UpdaterUtil::ensureDirectory(logs) &&
					QFileInfo(logs).isWritable())
					return logs;
			}
			return QDir::tempPath();
		}

	} // namespace

	void start(const QString& dataDir, bool rotate, bool mirrorToConsole)
	{
		QMutexLocker locker(&g_mutex);
		if (g_file != nullptr)
			return;

		const QString directory = resolveLogDirectory(dataDir);
		const QString path =
			QDir(directory).absoluteFilePath("meshmc-updater.log");
		const QString previous =
			QDir(directory).absoluteFilePath("meshmc-updater-1.log");

		if (rotate && QFileInfo::exists(path)) {
			QFile::remove(previous);
			QFile::rename(path, previous);
		}

		auto* file = new QFile(path);
		// Append: the second stage continues the first stage's story.
		if (!file->open(QIODevice::WriteOnly | QIODevice::Append |
						QIODevice::Text)) {
			delete file;
			// Last resort, so that --verbose at least still reports something.
			g_mirrorToConsole = true;
			g_previousHandler = qInstallMessageHandler(handler);
			return;
		}

		g_file = file;
		g_path = path;
		g_mirrorToConsole = mirrorToConsole;
		g_previousHandler = qInstallMessageHandler(handler);
	}

	QString filePath()
	{
		QMutexLocker locker(&g_mutex);
		return g_path;
	}

	void stop()
	{
		QMutexLocker locker(&g_mutex);
		qInstallMessageHandler(g_previousHandler);
		g_previousHandler = nullptr;
		if (g_file != nullptr) {
			g_file->flush();
			g_file->close();
			delete g_file;
			g_file = nullptr;
		}
	}

	bool attachToParentConsole()
	{
#ifdef Q_OS_WIN
		if (!AttachConsole(ATTACH_PARENT_PROCESS))
			return false;

		// AttachConsole hands us a console but not the C runtime's idea of one.
		FILE* stream = nullptr;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONOUT$", "w", stderr);
		return true;
#else
		return true; // a console is either already there or genuinely absent
#endif
	}

} // namespace UpdaterLog
