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

#include <QDateTime>
#include <QString>

//! What a lock file found on disk says about the update that wrote it.
struct UpdateLockInfo {
	bool present = false;
	QDateTime startedAt;
	QString stage;
	QString root;
	QString detail;
	qint64 pid = 0;

	//! True while the process that wrote the lock is still running.
	bool ownerAlive() const;

	//! Multi-line summary for the log and, if it comes to it, the user.
	QString describe() const;
};

/*!
 * Marks an installation as "being updated right now".
 *
 * Two things can go wrong without it. Two updates can be started at once --
 * easy enough with a tray icon and a menu entry -- and race each other over
 * the same files. And an update that dies halfway leaves an installation that
 * is neither the old version nor the new one, with nothing on disk saying so;
 * the next launch then looks like a mysterious corruption rather than a failed
 * update.
 *
 * The lock deliberately outlives the process that claimed it: the first stage
 * exits on purpose, handing the update -- and the lock -- to the second.
 * release() is therefore always explicit, never automatic.
 */
class UpdateLock
{
  public:
	explicit UpdateLock(const QString& dataDir);

	//! Read the lock file without touching it.
	UpdateLockInfo peek() const;

	/*!
	 * Take the lock for this process.
	 *
	 * Fails only if another *live* updater holds it. A lock left behind by a
	 * dead process is reported through \a takenOver and then overwritten:
	 * refusing to update because a previous attempt crashed would leave the
	 * user permanently stuck.
	 */
	bool claim(const QString& stage, const QString& root, const QString& detail,
			   UpdateLockInfo& takenOver);

	//! Record that the update moved on to another stage.
	void setStage(const QString& stage);

	//! Remove the lock file. Only correct once the update is truly finished.
	void release();

	//! Outcome of waitForTakeover().
	enum class Takeover {
		Accepted,  //!< Another process owns the lock now.
		Released,  //!< The update finished before we looked.
		OwnerGone, //!< \a childPid died without ever claiming it.
		TimedOut,  //!< Nobody claimed it in time.
	};

	/*!
	 * Block until some other process takes the lock over from \a ourPid.
	 *
	 * The second stage rewrites the lock under its own process id as its very
	 * first action, so the lock doubles as an "I am alive" signal and the
	 * first stage can tell "I started it" apart from "it actually ran".
	 *
	 * That distinction is not academic. The binary the first stage hands over
	 * to comes out of the downloaded release, so it is only ever as new as
	 * whatever is being installed -- and a release older than the two-stage
	 * updater does not understand its arguments, exits at once, and has no
	 * console to complain to. Without this check the first stage reports
	 * success, the launcher closes, and nothing whatsoever happens.
	 *
	 * \a childPid may be 0 if it is not known; the wait then relies purely on
	 * the timeout.
	 */
	Takeover waitForTakeover(qint64 ourPid, qint64 childPid, int timeoutMs,
							 int pollMs = 100) const;

	//! Human readable form of a waitForTakeover() outcome, for the log.
	static QString describeTakeover(Takeover outcome);

	QString path() const
	{
		return m_path;
	}

  private:
	void write(const QString& stage, const QString& root, const QString& detail,
			   const QDateTime& startedAt);

	QString m_path;
	QString m_root;
	QString m_detail;
	QDateTime m_startedAt;
};
