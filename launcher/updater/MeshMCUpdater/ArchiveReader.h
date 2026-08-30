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

/*!
 * Unpacking of downloaded update artifacts.
 *
 * There used to be one function per container -- zip and tar.gz -- differing
 * only in which libarchive format handlers they enabled, and drifting apart in
 * their error handling. libarchive can sniff the container itself, so there is
 * one implementation here and the file extension no longer decides anything.
 */
namespace ArchiveReader
{

	struct Result {
		bool ok = false;
		QString error; //!< Set when ok is false.
		int fileCount = 0;
		qint64 byteCount = 0;
	};

	/*!
	 * Extract \a archivePath into \a destDir, which is created if needed.
	 *
	 * Entries whose destination would land outside \a destDir are rejected and
	 * abort the extraction. The artifact is fetched over the network, so it is
	 * not treated as trustworthy input no matter who is supposed to have
	 * published it.
	 */
	Result extract(const QString& archivePath, const QString& destDir);

	/*!
	 * If \a dir holds exactly one entry and that entry is a directory, return
	 * it; otherwise return \a dir.
	 *
	 * Release zips are built both ways -- files at the top level, or everything
	 * inside a single versioned folder -- and the caller wants the directory
	 * that actually contains the application.
	 */
	QString descendIntoSingleRoot(const QString& dir);

} // namespace ArchiveReader
