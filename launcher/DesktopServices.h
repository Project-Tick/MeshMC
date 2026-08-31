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

#include <QUrl>
#include <QString>

/**
 * This wraps around QDesktopServices and adds workarounds where needed
 * Use this instead of QDesktopServices!
 */
namespace DesktopServices
{
	/**
	 * Open a file in whatever application is applicable
	 */
	bool openFile(const QString& path);

	/**
	 * Open a file in the specified application
	 */
	bool openFile(const QString& application, const QString& path,
				  const QString& workingDirectory = QString(), qint64* pid = 0);

	/**
	 * Run an application
	 */
	bool run(const QString& application, const QStringList& args,
			 const QString& workingDirectory = QString(), qint64* pid = 0);

	/**
	 * Open a directory
	 */
	bool openDirectory(const QString& path, bool ensureExists = false);

	/**
	 * Open the URL, most likely in a browser. Maybe.
	 */
	bool openUrl(const QUrl& url);

	/**
	 * Whether this process is running inside a Flatpak sandbox.
	 *
	 * It matters wherever the launcher has to name or reach itself from the
	 * outside: its own executable path means nothing to the host, so a
	 * desktop entry has to go through `flatpak run` instead, and a file can
	 * only be written outside the sandbox through the portal's own save
	 * dialog.
	 */
	bool isFlatpak();
} // namespace DesktopServices
