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

#include "UpdaterOptions.h"

#include <QObject>
#include <QString>
#include <QStringList>

class UpdateLock;

/*!
 * Second half of an update: put the unpacked files in place.
 *
 * Runs from the staging directory, out of the new release's own copy of Qt, so
 * the installation it is overwriting is not mapped by anything. That is the
 * whole reason this is a separate process, and the reason it can replace
 * Qt6Core.dll -- something the first stage could never do to itself.
 *
 * It does not delete the staging directory it is running from; the next
 * update's first stage clears it, by which time nothing has it open.
 */
class ApplyStage : public QObject
{
	Q_OBJECT

  public:
	ApplyStage(const UpdaterOptions& options, UpdateLock& lock,
			   QObject* parent = nullptr);

	//! Run to completion. Emits finished() exactly once.
	void start();

  signals:
	void progress(const QString& message);
	void finished(bool ok, const QString& error);

  private:
	/*!
	 * Copy every file the update is about to overwrite into a timestamped
	 * backup directory.
	 *
	 * Only files that are actually being replaced are copied. A portable
	 * installation keeps instances, saves and configuration inside the install
	 * root, and none of that is ours to duplicate.
	 */
	bool backup(const QStringList& incoming, QString& error);

	bool installFiles(const QStringList& incoming, QString& error);

	//! Keep the last few backups, drop the rest.
	void pruneBackups();

	void relaunch();

	const UpdaterOptions& m_options;
	UpdateLock& m_lock;
	QString m_backupDir;
};
