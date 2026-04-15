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
