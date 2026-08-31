/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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
#include "updater/UpdateChecker.h"

namespace Ui
{
	class UpdateDialog;
}

enum UpdateAction {
	UPDATE_LATER = QDialog::Rejected,
	UPDATE_NOW = QDialog::Accepted,
};

class UpdateDialog : public QDialog
{
	Q_OBJECT

  public:
	/*!
	 * Constructs the update dialog.
	 * \a hasUpdate    - true when an update is available (shows "Update now"
	 * button).
	 * \a status       - update information (version, release notes); ignored
	 * when hasUpdate is false.
	 */
	explicit UpdateDialog(bool hasUpdate,
						  const UpdateAvailableStatus& status = {},
						  QWidget* parent = nullptr);
	~UpdateDialog();

  public slots:
	void on_btnUpdateNow_clicked();
	void on_btnUpdateLater_clicked();

  protected:
	void closeEvent(QCloseEvent*) override;

  private:
	Ui::UpdateDialog* ui;
};
