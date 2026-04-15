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

#include "tasks/Task.h"
#include "net/NetJob.h"
#include "java/download/JavaRuntime.h"

#include <QUrl>

class JavaDownloadTask : public Task
{
	Q_OBJECT

  public:
	explicit JavaDownloadTask(const JavaDownload::RuntimeEntry& runtime,
							  const QString& targetDir,
							  QObject* parent = nullptr);
	virtual ~JavaDownloadTask() = default;

	QString installedJavaPath() const
	{
		return m_installedJavaPath;
	}

  protected:
	void executeTask() override;

  private slots:
	void downloadFinished();
	void downloadFailed(QString reason);
	void extractArchive();
	void manifestDownloaded();
	void manifestFilesDownloaded();

  private:
	void downloadArchive();
	void downloadManifest();
	QString findJavaBinary(const QString& dir) const;

	JavaDownload::RuntimeEntry m_runtime;
	QString m_targetDir;
	QString m_archivePath;
	QString m_installedJavaPath;
	NetJob::Ptr m_downloadJob;
	QByteArray m_manifestData;
	QStringList m_executableFiles;
	QList<QPair<QString, QString>> m_linkEntries;
};
