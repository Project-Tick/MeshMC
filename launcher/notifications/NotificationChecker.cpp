/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "NotificationChecker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

#include "net/Download.h"

#include "Application.h"

NotificationChecker::NotificationChecker(QObject* parent) : QObject(parent) {}

void NotificationChecker::setNotificationsUrl(const QUrl& notificationsUrl)
{
	m_notificationsUrl = notificationsUrl;
}

void NotificationChecker::setApplicationChannel(QString channel)
{
	m_appVersionChannel = channel;
}

void NotificationChecker::setApplicationFullVersion(QString version)
{
	m_appFullVersion = version;
}

void NotificationChecker::setApplicationPlatform(QString platform)
{
	m_appPlatform = platform;
}

QList<NotificationChecker::NotificationEntry>
NotificationChecker::notificationEntries() const
{
	return m_entries;
}

void NotificationChecker::checkForNotifications()
{
	if (!m_notificationsUrl.isValid()) {
		qCritical()
			<< "Failed to check for notifications. No notifications URL set."
			<< "If you'd like to use MeshMC's notification system, please pass "
			   "the "
			   "URL to CMake at compile time.";
		return;
	}
	if (m_checkJob) {
		return;
	}
	m_checkJob =
		new NetJob("Checking for notifications", APPLICATION->network());
	auto entry =
		APPLICATION->metacache()->resolveEntry("root", "notifications.json");
	entry->setStale(true);
	m_checkJob->addNetAction(
		m_download = Net::Download::makeCached(m_notificationsUrl, entry));
	connect(m_download.get(), &Net::Download::succeeded, this,
			&NotificationChecker::downloadSucceeded);
	m_checkJob->start();
}

void NotificationChecker::downloadSucceeded(int)
{
	m_entries.clear();

	QFile file(m_download->getTargetFilepath());
	if (file.open(QFile::ReadOnly)) {
		QJsonArray root = QJsonDocument::fromJson(file.readAll()).array();
		for (auto it = root.begin(); it != root.end(); ++it) {
			QJsonObject obj = (*it).toObject();
			NotificationEntry entry;
			entry.id = obj.value("id").toDouble();
			entry.message = obj.value("message").toString();
			entry.channel = obj.value("channel").toString();
			entry.platform = obj.value("platform").toString();
			entry.from = obj.value("from").toString();
			entry.to = obj.value("to").toString();
			const QString type = obj.value("type").toString("critical");
			if (type == "critical") {
				entry.type = NotificationEntry::Critical;
			} else if (type == "warning") {
				entry.type = NotificationEntry::Warning;
			} else if (type == "information") {
				entry.type = NotificationEntry::Information;
			}
			if (entryApplies(entry))
				m_entries.append(entry);
		}
	}

	m_checkJob.reset();

	emit notificationCheckFinished();
}

bool versionLessThan(const QString& v1, const QString& v2)
{
	QStringList l1 = v1.split('.');
	QStringList l2 = v2.split('.');
	while (!l1.isEmpty() && !l2.isEmpty()) {
		int one = l1.isEmpty() ? 0 : l1.takeFirst().toInt();
		int two = l2.isEmpty() ? 0 : l2.takeFirst().toInt();
		if (one != two) {
			return one < two;
		}
	}
	return false;
}

bool NotificationChecker::entryApplies(
	const NotificationChecker::NotificationEntry& entry) const
{
	bool channelApplies =
		entry.channel.isEmpty() || entry.channel == m_appVersionChannel;
	bool platformApplies =
		entry.platform.isEmpty() || entry.platform == m_appPlatform;
	bool fromApplies = entry.from.isEmpty() || entry.from == m_appFullVersion ||
					   !versionLessThan(m_appFullVersion, entry.from);
	bool toApplies = entry.to.isEmpty() || entry.to == m_appFullVersion ||
					 !versionLessThan(entry.to, m_appFullVersion);
	return channelApplies && platformApplies && fromApplies && toApplies;
}
