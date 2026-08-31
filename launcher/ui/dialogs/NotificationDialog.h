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

#ifndef NOTIFICATIONDIALOG_H
#define NOTIFICATIONDIALOG_H

#include <QDialog>

#include "notifications/NotificationChecker.h"

namespace Ui
{
	class NotificationDialog;
}

class NotificationDialog : public QDialog
{
	Q_OBJECT

  public:
	explicit NotificationDialog(
		const NotificationChecker::NotificationEntry& entry,
		QWidget* parent = 0);
	~NotificationDialog();

	enum ExitCode { Normal, DontShowAgain };

  protected:
	void timerEvent(QTimerEvent* event);

  private:
	Ui::NotificationDialog* ui;

	int m_dontShowAgainTime = 10;
	int m_closeTime = 5;

	QString m_dontShowAgainText;
	QString m_closeText;

  private slots:
	void on_dontShowAgainBtn_clicked();
	void on_closeBtn_clicked();
};

#endif // NOTIFICATIONDIALOG_H
