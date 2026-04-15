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

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

/*!
 * Installer downloads an update artifact and installs it.
 *
 * Supported artifact types (determined by file extension):
 *   .exe   - Windows NSIS/Inno installer: run directly, installer handles
 * everything. .zip   - Portable zip archive: extract over the root directory.
 *   .tar.gz / .tgz - Portable tarball: extract over the root directory.
 *   .dmg   - macOS disk image: mount, copy .app bundle, unmount. (planned)
 *
 * After a successful archive extraction the application binary given by
 * \c setRelaunchPath() is started and the updater exits.
 */
class Installer : public QObject
{
	Q_OBJECT

  public:
	explicit Installer(QObject* parent = nullptr);

	void setDownloadUrl(const QString& url)
	{
		m_url = url;
	}
	void setRootPath(const QString& root)
	{
		m_root = root;
	}
	void setRelaunchPath(const QString& exec)
	{
		m_exec = exec;
	}

	/*!
	 * Start the install process asynchronously.
	 * The object emits \c finished() with a success flag when done.
	 */
	void start();

  signals:
	void progressMessage(const QString& msg);
	void finished(bool success, const QString& errorMessage);

  private slots:
	void onDownloadProgress(qint64 received, qint64 total);
	void onDownloadFinished();

  private:
	bool installArchive(const QString& filePath);
	bool installExe(const QString& filePath);
	bool extractZip(const QString& zipPath, const QString& destDir);
	bool extractTarGz(const QString& tarPath, const QString& destDir);
	void relaunch();

	QString m_url;
	QString m_root;
	QString m_exec;
	QString m_tempFile;

	QNetworkAccessManager* m_nam = nullptr;
};
