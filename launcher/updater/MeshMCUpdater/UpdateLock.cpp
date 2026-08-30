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

#include "UpdateLock.h"

#include "UpdaterUtil.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QThread>

namespace
{

	constexpr auto kLockFileName = "update.lock";

	constexpr auto kKeyStartedAt = "startedAt";
	constexpr auto kKeyStage = "stage";
	constexpr auto kKeyRoot = "root";
	constexpr auto kKeyDetail = "detail";
	constexpr auto kKeyPid = "pid";

} // namespace

bool UpdateLockInfo::ownerAlive() const
{
	return present && UpdaterUtil::isProcessRunning(pid);
}

QString UpdateLockInfo::describe() const
{
	if (!present)
		return QStringLiteral("no update in progress");

	return QStringLiteral("started %1 by pid %2, stage \"%3\", root \"%4\"%5")
		.arg(startedAt.isValid() ? startedAt.toString(Qt::ISODate)
								 : QStringLiteral("at an unknown time"),
			 QString::number(pid), stage, root,
			 detail.isEmpty() ? QString() : QStringLiteral(", %1").arg(detail));
}

UpdateLock::UpdateLock(const QString& dataDir)
	: m_path(QDir(dataDir).absoluteFilePath(QLatin1String(kLockFileName)))
{
}

UpdateLockInfo UpdateLock::peek() const
{
	UpdateLockInfo info;
	if (!QFileInfo::exists(m_path))
		return info;

	QSettings file(m_path, QSettings::IniFormat);
	info.present = true;
	info.startedAt = QDateTime::fromString(
		file.value(QLatin1String(kKeyStartedAt)).toString(), Qt::ISODate);
	info.stage = file.value(QLatin1String(kKeyStage)).toString();
	info.root = file.value(QLatin1String(kKeyRoot)).toString();
	info.detail = file.value(QLatin1String(kKeyDetail)).toString();
	info.pid = file.value(QLatin1String(kKeyPid)).toLongLong();
	return info;
}

bool UpdateLock::claim(const QString& stage, const QString& root,
					   const QString& detail, UpdateLockInfo& takenOver)
{
	const UpdateLockInfo existing = peek();
	if (existing.ownerAlive()) {
		takenOver = existing;
		return false;
	}
	if (existing.present) {
		// Left behind by an update that died. Say so loudly -- the
		// installation may be half written -- but do not block the user.
		takenOver = existing;
		qWarning() << "UpdateLock: taking over a stale lock:"
				   << existing.describe();
	}

	m_root = root;
	m_detail = detail;
	m_startedAt = QDateTime::currentDateTime();
	write(stage, root, detail, m_startedAt);
	return true;
}

void UpdateLock::setStage(const QString& stage)
{
	if (!m_startedAt.isValid()) {
		// Adopted from the previous stage rather than claimed here.
		const UpdateLockInfo existing = peek();
		m_startedAt = existing.startedAt.isValid()
						  ? existing.startedAt
						  : QDateTime::currentDateTime();
		if (m_root.isEmpty())
			m_root = existing.root;
		if (m_detail.isEmpty())
			m_detail = existing.detail;
	}
	write(stage, m_root, m_detail, m_startedAt);
}

UpdateLock::Takeover UpdateLock::waitForTakeover(qint64 ourPid, qint64 childPid,
												 int timeoutMs,
												 int pollMs) const
{
	QElapsedTimer timer;
	timer.start();

	for (;;) {
		const UpdateLockInfo current = peek();
		if (!current.present)
			return Takeover::Released;
		if (current.pid != 0 && current.pid != ourPid)
			return Takeover::Accepted;

		// Liveness is checked only after inspecting the lock: a process that
		// claimed it and then finished very quickly has still done its job,
		// and must not be reported as having died on us.
		if (childPid > 0 && !UpdaterUtil::isProcessRunning(childPid))
			return Takeover::OwnerGone;

		if (timer.hasExpired(timeoutMs))
			return Takeover::TimedOut;

		QThread::msleep(static_cast<unsigned long>(qMax(1, pollMs)));
	}
}

QString UpdateLock::describeTakeover(Takeover outcome)
{
	switch (outcome) {
		case Takeover::Accepted:
			return QStringLiteral("another process took the update over");
		case Takeover::Released:
			return QStringLiteral("the update was already finished");
		case Takeover::OwnerGone:
			return QStringLiteral("the process we started exited without "
								  "taking the update over");
		case Takeover::TimedOut:
			return QStringLiteral("nobody took the update over in time");
	}
	return QStringLiteral("unknown outcome");
}

void UpdateLock::release()
{
	if (!QFile::remove(m_path) && QFileInfo::exists(m_path))
		qWarning() << "UpdateLock: could not remove" << m_path;
}

void UpdateLock::write(const QString& stage, const QString& root,
					   const QString& detail, const QDateTime& startedAt)
{
	if (!UpdaterUtil::ensureDirectory(QFileInfo(m_path).absolutePath())) {
		qWarning() << "UpdateLock: cannot create"
				   << QFileInfo(m_path).absolutePath();
		return;
	}

	QSettings file(m_path, QSettings::IniFormat);
	file.setValue(QLatin1String(kKeyStartedAt),
				  startedAt.toString(Qt::ISODate));
	file.setValue(QLatin1String(kKeyStage), stage);
	file.setValue(QLatin1String(kKeyRoot), root);
	file.setValue(QLatin1String(kKeyDetail), detail);
	file.setValue(QLatin1String(kKeyPid), QCoreApplication::applicationPid());
	file.sync();

	if (file.status() != QSettings::NoError)
		qWarning() << "UpdateLock: failed to write" << m_path;
}
