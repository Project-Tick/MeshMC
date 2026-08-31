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
 * Which half of the update this process is responsible for.
 *
 * A single-process updater cannot work on Windows: the files it must replace
 * include the very libraries it has mapped (Qt6Core.dll, Qt6Network.dll, the
 * MSVC runtime, the TLS backend plugin). A mapped image can be renamed but
 * never deleted, so an in-place updater reliably dies partway through the
 * copy and leaves a half-written installation behind -- worse than not
 * updating at all.
 *
 * The work is therefore split across two processes:
 *
 *   Prepare  runs from the *installed* copy. It downloads the artifact and
 *            unpacks it into a staging directory, then starts the updater it
 *            just unpacked and exits. Exiting is the point: it releases every
 *            lock this process held on the installation.
 *
 *   Apply    runs from the *staging* directory. Nothing in the install root
 *            is mapped by this process, so it can overwrite all of it,
 *            including the libraries Prepare was using.
 */
enum class UpdaterStage {
	Prepare,
	Apply,
};

/*!
 * The updater's command line, after sanitising.
 *
 * Parsing is kept free of side effects so it can be exercised directly by the
 * unit test; nothing here touches the filesystem except validate(), which only
 * reads.
 */
struct UpdaterOptions {
	UpdaterStage stage = UpdaterStage::Prepare;

	QString url;	 //!< Prepare: artifact to download.
	QString source;	 //!< Apply: staging directory to install from.
	QString root;	 //!< Installation root to update.
	QString exec;	 //!< Binary to start once the update is done.
	QString dataDir; //!< Holds the log, the lock file and the staging area.

	qint64 waitPid = 0;	  //!< Outlive this process before touching files.
	bool verbose = false; //!< Mirror the log to the parent console too.
	bool helpRequested = false;

	QString helpText; //!< Filled in by parse(), for --help.

	/*!
	 * Remove wrapping quote characters that the shell failed to strip.
	 *
	 * cmd.exe does not treat ' as a quote at all, so
	 *
	 *     --root 'C:\Users\me\AppData\Local\Programs\MeshMC\'
	 *
	 * arrives with the apostrophes still attached. Every path derived from it
	 * then refers to a directory that cannot exist, and the updater fails in a
	 * place far away from the actual mistake.
	 *
	 * Unmatched quotes are stripped as well, because cmd.exe produces those on
	 * its own: in `--root "C:\dir\"` the backslash escapes the closing quote,
	 * so the argument arrives as `C:\dir"` with a single trailing quote.
	 */
	static QString unquote(const QString& value);

	//! unquote(), then QDir::cleanPath(), then make absolute.
	static QString cleanPathArgument(const QString& value);

	/*!
	 * Parse an argv-style list; \a arguments[0] is the program name.
	 *
	 * Returns false and fills \a error when the command line is malformed.
	 * A well-formed command line may still be unusable -- call validate()
	 * for that, once logging is up, so the reason reaches the log file.
	 */
	bool parse(const QStringList& arguments, QString& error);

	//! Returns a human readable problem, or an empty string when usable.
	QString validate() const;

	//! Command line that hands this update over to the Apply stage.
	QStringList applyArguments(const QString& stagingDir,
							   qint64 prepareStagePid) const;
};
