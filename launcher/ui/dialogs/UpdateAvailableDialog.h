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

#include <QDialog>

namespace Ui
{
	class UpdateAvailableDialog;
}

class UpdateAvailableDialog : public QDialog
{
	Q_OBJECT

  public:
	enum ResultCode {
		Install = 10,
		DontInstall = 11,
		Skip = 12,
	};

	/*!
	 * \a currentVersion  version the user is running, as displayed.
	 * \a availableVersion version being offered, as displayed.
	 * \a releaseNotes    release notes in Markdown, as published.
	 */
	explicit UpdateAvailableDialog(const QString& currentVersion,
								   const QString& availableVersion,
								   const QString& releaseNotes,
								   QWidget* parent = nullptr);
	~UpdateAvailableDialog() override;

  private:
	Ui::UpdateAvailableDialog* ui;
};
