/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Project Tick
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
