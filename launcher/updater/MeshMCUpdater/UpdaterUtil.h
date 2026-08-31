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
#include <QStringList>

/*!
 * Small filesystem and process helpers for the standalone updater.
 *
 * The launcher's FS:: namespace would do most of this, but pulling it in would
 * drag MeshMC_logic -- and with it Qt Widgets, Xml, NetworkAuth and the whole
 * plugin system -- into a binary whose entire job is to outlive as little as
 * possible. The updater stays deliberately small and self-contained.
 */
namespace UpdaterUtil
{

	//! Suffix left behind when a file could only be renamed out of the way.
	inline constexpr auto kDisplacedSuffix = ".meshmc-old";

	// Sub-directories of the data directory that an update works in. They live
	// next to the launcher's own data rather than inside the install root, so a
	// failed update leaves no debris in the installation itself -- and nothing
	// that would end up listed in the next release's manifest.
	inline constexpr auto kDownloadDirName = "update-download";
	inline constexpr auto kStagingDirName = "update-staging";
	inline constexpr auto kBackupDirName = "update-backup";

	//! True while the given process id still refers to a live process.
	bool isProcessRunning(qint64 pid);

	/*!
	 * Block until \a pid exits, or \a timeoutMs elapses.
	 *
	 * Returns true when the process is gone (including when it was already
	 * gone, or when \a pid is 0). This is what makes the hand-off deterministic
	 * instead of the usual "sleep a couple of seconds and hope".
	 */
	bool waitForProcessExit(qint64 pid, int timeoutMs);

	//! mkpath(), reported as a bool so callers can produce a useful message.
	bool ensureDirectory(const QString& path);

	/*!
	 * Copy \a from onto \a to, replacing whatever is there.
	 *
	 * Retries a few times, because a virus scanner or a search indexer can hold
	 * a freshly written file open for a moment. If the destination cannot be
	 * deleted at all -- the case for a DLL some process still has mapped -- it
	 * is renamed aside with the kDisplacedSuffix suffix, which Windows permits
	 * even for a mapped image, and the copy proceeds.
	 *
	 * Returns false and fills \a error when even that does not work.
	 */
	bool installFile(const QString& from, const QString& to, QString& error);

	//! Paths, relative to \a dir, of every file below it. Directories are
	//! implied.
	QStringList relativeFilePaths(const QString& dir);

	//! Delete a directory and everything in it. Returns false if anything
	//! stayed.
	bool removeDirectoryTree(const QString& path);

	//! Delete files left behind by installFile()'s rename fallback.
	int sweepDisplacedFiles(const QString& root);

	//! "12.3 MiB", for the log.
	QString formatBytes(qint64 bytes);

} // namespace UpdaterUtil
