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

class QWidget;

/*!
 * UpdateController launches the separate meshmc-updater binary and then
 * requests the main application to quit so the updater can proceed.
 *
 * The updater binary is located next to the running executable
 * (QApplication::applicationDirPath()).  It receives:
 *   --url  <download_url>   - artifact to download and install
 *   --root <root_path>      - installation root (prefix directory)
 *   --exec <app_binary>     - path to re-launch after the update completes
 */
class UpdateController
{
  public:
	UpdateController(QWidget* parent, const QString& root,
					 const QString& downloadUrl);

	/*!
	 * Locates the meshmc-updater binary next to the running executable,
	 * launches it with the required arguments, and returns true on success.
	 * The caller is responsible for quitting the main application afterwards.
	 */
	bool startUpdate();

  private:
	QWidget* m_parent;
	QString m_root;
	QString m_downloadUrl;
};
