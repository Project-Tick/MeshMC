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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Exception.h"
#include "pathmatcher/IPathMatcher.h"

#include <QDir>
#include <QFlags>

namespace FS
{

	class FileSystemException : public ::Exception
	{
	  public:
		FileSystemException(const QString& message) : Exception(message) {}
	};

	/**
	 * write data to a file safely
	 */
	void write(const QString& filename, const QByteArray& data);

	/**
	 * read data from a file safely\
	 */
	QByteArray read(const QString& filename);

	/**
	 * Update the last changed timestamp of an existing file
	 */
	bool updateTimestamp(const QString& filename);

	/**
	 * Creates all the folders in a path for the specified path
	 * last segment of the path is treated as a file name and is ignored!
	 */
	bool ensureFilePathExists(QString filenamepath);

	/**
	 * Creates all the folders in a path for the specified path
	 * last segment of the path is treated as a folder name and is created!
	 */
	bool ensureFolderPathExists(QString filenamepath);

	class copy
	{
	  public:
		copy(const QString& src, const QString& dst)
		{
			m_src = src;
			m_dst = dst;
		}
		copy& followSymlinks(const bool follow)
		{
			m_followSymlinks = follow;
			return *this;
		}
		copy& blacklist(const IPathMatcher* filter)
		{
			m_blacklist = filter;
			return *this;
		}
		bool operator()()
		{
			return operator()(QString());
		}

	  private:
		bool operator()(const QString& offset);

	  private:
		bool m_followSymlinks = true;
		const IPathMatcher* m_blacklist = nullptr;
		QDir m_src;
		QDir m_dst;
	};

	/**
	 * Delete a folder recursively
	 */
	bool deletePath(QString path);

	/**
	 * Whether this platform has a trash we can trust to be reversible.
	 *
	 * Worth asking before offering the user a choice, so that a dialog
	 * can promise "recoverable" or "permanent" and be right either way
	 * rather than hedging.
	 */
	bool canTrash();

	/**
	 * Move a file or folder to the platform's trash, so that the user can
	 * put it back.
	 *
	 * On success @p pathInTrash, when given, receives where the item
	 * landed, which is what makes an undo possible.
	 *
	 * Returns false without touching anything where there is no trash we
	 * can trust to be reversible; the caller is then free to fall back to
	 * deletePath() -- but it has to say so, because at that point the
	 * operation is no longer undoable.
	 */
	bool trash(const QString& path, QString* pathInTrash = nullptr);

	QString PathCombine(const QString& path1, const QString& path2);
	QString PathCombine(const QString& path1, const QString& path2,
						const QString& path3);
	QString PathCombine(const QString& path1, const QString& path2,
						const QString& path3, const QString& path4);

	QString AbsolutePath(QString path);

	/**
	 * Resolve an executable
	 *
	 * Will resolve:
	 *   single executable (by name)
	 *   relative path
	 *   absolute path
	 *
	 * @return absolute path to executable or null string
	 */
	QString ResolveExecutable(QString path);

	/**
	 * Normalize path
	 *
	 * Any paths inside the current directory will be normalized to relative
	 * paths (to current) Other paths will be made absolute
	 *
	 * Returns false if the path logic somehow filed (and normalizedPath in
	 * invalid)
	 */
	QString NormalizePath(QString path);

	QString RemoveInvalidFilenameChars(QString string, QChar replaceWith = '-');

	QString DirNameFromString(QString string, QString inDir = ".");

	/// Checks if the a given Path contains "!"
	bool checkProblemticPathJava(QDir folder);

	// Get the Directory representing the User's Desktop
	QString getDesktopDir();

	// Get the directory the platform lists installed applications in
	QString getApplicationsDir();

	/**
	 * Write a shortcut that runs @p target with @p args, labelled @p name and
	 * wearing the image at @p iconPath.
	 *
	 * @p destination is the path to create *without* a suffix -- each
	 * platform appends its own, because what a shortcut even is differs: a
	 * `.lnk` shell link on Windows, a `.desktop` entry on Linux and the BSDs,
	 * and a small `.app` bundle on macOS, that being the only form Finder
	 * will show with an icon and arguments of its own.
	 *
	 * Returns the path actually written, or an empty string if nothing was.
	 */
	QString createShortcut(QString destination, QString target,
						   QStringList args, QString name, QString iconPath);
} // namespace FS
