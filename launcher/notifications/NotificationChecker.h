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

#pragma once

#include <QObject>

#include "net/NetJob.h"
#include "net/Download.h"

class NotificationChecker : public QObject
{
	Q_OBJECT

  public:
	explicit NotificationChecker(QObject* parent = 0);

	void setNotificationsUrl(const QUrl& notificationsUrl);
	void setApplicationPlatform(QString platform);
	void setApplicationChannel(QString channel);
	void setApplicationFullVersion(QString version);

	struct NotificationEntry {
		int id;
		QString message;
		enum { Critical, Warning, Information } type;
		QString channel;
		QString platform;
		QString from;
		QString to;
	};

	QList<NotificationEntry> notificationEntries() const;

  public slots:
	void checkForNotifications();

  private slots:
	void downloadSucceeded(int);

  signals:
	void notificationCheckFinished();

  private:
	bool entryApplies(const NotificationEntry& entry) const;

  private:
	QList<NotificationEntry> m_entries;
	QUrl m_notificationsUrl;
	NetJob::Ptr m_checkJob;
	Net::Download::Ptr m_download;

	QString m_appVersionChannel;
	QString m_appPlatform;
	QString m_appFullVersion;
};
