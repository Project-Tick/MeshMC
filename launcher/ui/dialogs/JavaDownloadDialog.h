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

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QSplitter>

#include "java/download/JavaRuntime.h"
#include "java/download/JavaDownloadTask.h"
#include "net/NetJob.h"

class JavaDownloadDialog : public QDialog
{
	Q_OBJECT

  public:
	explicit JavaDownloadDialog(QWidget* parent = nullptr);
	~JavaDownloadDialog() override = default;

	QString installedJavaPath() const
	{
		return m_installedJavaPath;
	}

  private slots:
	void providerChanged(int index);
	void majorVersionChanged(int index);
	void subVersionChanged(int index);
	void onDownloadClicked();
	void onCancelClicked();

  private:
	void setupUi();
	void fetchVersionList(const QString& uid);
	void fetchRuntimes(const QString& uid, const QString& versionId);
	QString javaInstallDir() const;

	// Left panel: providers
	QListWidget* m_providerList = nullptr;
	// Center panel: major versions (Java 25, Java 21, ...)
	QListWidget* m_versionList = nullptr;
	// Right panel: sub-versions / builds
	QListWidget* m_subVersionList = nullptr;

	QLabel* m_infoLabel = nullptr;
	QLabel* m_statusLabel = nullptr;
	QPushButton* m_downloadBtn = nullptr;
	QPushButton* m_cancelBtn = nullptr;
	QProgressBar* m_progressBar = nullptr;

	QList<JavaDownload::JavaProviderInfo> m_providers;
	QList<JavaDownload::JavaVersionInfo> m_versions;
	QList<JavaDownload::RuntimeEntry> m_runtimes;

	NetJob::Ptr m_fetchJob;
	QByteArray m_fetchData;
	std::unique_ptr<JavaDownloadTask> m_downloadTask;
	QString m_installedJavaPath;
};
