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
 * The updater's log file.
 *
 * On Windows the updater is a GUI-subsystem binary, so it has no console:
 * qDebug(), qCritical() and fprintf(stderr) all go nowhere, and a failed
 * update is indistinguishable from a successful one. Everything the updater
 * says therefore goes to a file, from the first line onwards.
 *
 * Both stages append to the same file so a whole update reads as one story.
 * The previous run is kept alongside it, because the interesting failure is
 * usually the one before the user thought to look.
 */
namespace UpdaterLog
{

	/*!
	 * Start logging. Call once, as early as possible.
	 *
	 * \a dataDir is where the log belongs; if it cannot be written the
	 * temporary directory is used instead, so there is always somewhere to
	 * look. \a rotate should be true for the first stage of an update and false
	 * for the second, which continues the same story.
	 *
	 * \a mirrorToConsole additionally writes to the console that started the
	 * process, when there is one -- see attachToParentConsole().
	 */
	void start(const QString& dataDir, bool rotate, bool mirrorToConsole);

	//! Where start() ended up writing. Shown to the user when an update fails.
	QString filePath();

	//! Flush and uninstall the handler. Called from main() on the way out.
	void stop();

	/*!
	 * Windows only: adopt the console of the process that started us, so a
	 * developer running the updater by hand actually sees its output. Does
	 * nothing when there is no parent console, and nothing at all elsewhere.
	 */
	bool attachToParentConsole();

} // namespace UpdaterLog
