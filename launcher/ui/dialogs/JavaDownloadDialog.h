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
