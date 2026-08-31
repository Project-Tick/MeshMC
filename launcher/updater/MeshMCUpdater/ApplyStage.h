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
