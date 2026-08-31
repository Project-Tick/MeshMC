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

class QFile;
class QNetworkAccessManager;
class QNetworkReply;
class UpdateLock;

/*!
 * First half of an update: fetch the new version and hand it the job.
 *
 * Runs from the installation being replaced, and therefore never writes to it.
 * Everything lands in the data directory instead. Its last act is to start the
 * updater it just unpacked and quit, which is what frees the libraries the
 * second stage needs to overwrite.
 *
 * The one thing this stage does to the install root is sweep up files a
 * previous update had to rename out of the way; by now nothing has them open.
 */
class PrepareStage : public QObject
{
	Q_OBJECT

  public:
	PrepareStage(const UpdaterOptions& options, UpdateLock& lock,
				 QObject* parent = nullptr);

	//! Begin. Emits finished() exactly once, possibly before returning.
	void start();

  signals:
	//! Human readable progress, for the log.
	void progress(const QString& message);

	/*!
	 * \a handedOff is true when the second stage took over, which means the
	 * update is still running and the lock must stay where it is.
	 */
	void finished(bool ok, const QString& error, bool handedOff);

  private slots:
	void onDownloadReadyRead();
	void onDownloadProgress(qint64 received, qint64 total);
	void onDownloadFinished();

  private:
	void beginDownload();
	void unpackAndHandOff();
	bool runBundledInstaller();
	bool handOffTo(const QString& stagingDir);

	/*!
	 * Block until the second stage confirms it has taken over, or give up.
	 *
	 * The updater we hand to comes out of the downloaded release, so it is
	 * only as capable as the version being installed; anything older than the
	 * two-stage mechanism rejects these arguments and exits without a word.
	 * Reporting "handed over successfully" in that case is exactly how an
	 * update fails in complete silence, so the hand-off is confirmed rather
	 * than assumed -- the second stage takes the lock under its own process id
	 * before doing anything else, and that is the signal.
	 */
	bool waitForApplyStage(qint64 applyPid);

	void fail(const QString& error);

	//! Guard against unpacking something that is not a MeshMC installation.
	QString describeStagingProblem(const QString& stagingDir) const;

	QString updaterBinaryName() const;

	const UpdaterOptions& m_options;
	UpdateLock& m_lock;

	QNetworkAccessManager* m_network = nullptr;
	QNetworkReply* m_reply = nullptr;
	QFile* m_download = nullptr;
	QString m_downloadPath;
	int m_lastReportedPercent = -1;
};
