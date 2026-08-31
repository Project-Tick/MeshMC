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
