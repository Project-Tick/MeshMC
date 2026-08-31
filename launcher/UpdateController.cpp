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

#include "UpdateController.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>

#include "BuildConfig.h"
#include "FileSystem.h"

UpdateController::UpdateController(QWidget* parent, const QString& root,
								   const QString& downloadUrl)
	: m_parent(parent), m_root(root), m_downloadUrl(downloadUrl)
{
}

bool UpdateController::startUpdate()
{
	// Locate the updater binary next to the running executable.
	QString updaterName = BuildConfig.MESHMC_BINARY + "-updater";
#ifdef Q_OS_WIN
	updaterName += ".exe";
#endif
	const QString updaterPath =
		FS::PathCombine(QApplication::applicationDirPath(), updaterName);

	if (!QFile::exists(updaterPath)) {
		qCritical() << "UpdateController: updater binary not found at"
					<< updaterPath;
		QMessageBox::critical(
			m_parent,
			QCoreApplication::translate("UpdateController",
										"Updater Not Found"),
			QCoreApplication::translate("UpdateController",
										"The updater binary could not be found "
										"at:\n%1\n\nPlease reinstall %2.")
				.arg(updaterPath, BuildConfig.MESHMC_DISPLAYNAME));
		return false;
	}

	// The updater keeps its log, its lock file and the staging area next to
	// our own data instead of inside the installation, so that a failed update
	// leaves no debris behind -- and nothing that would show up in the next
	// release's manifest. resolveDataPath() makes the data path the working
	// directory (Application.cpp, QDir::setCurrent) before anything else runs.
	const QString dataDir = QDir::currentPath();

	// Hand over our process id so the updater can wait for us to actually be
	// gone rather than sleeping for a couple of seconds and hoping. MainWindow
	// quits the application as soon as this returns.
	const QStringList args = {
		"--url",	  m_downloadUrl,
		"--root",	  m_root,
		"--exec",	  QApplication::applicationFilePath(),
		"--data-dir", dataDir,
		"--wait-pid", QString::number(QCoreApplication::applicationPid())};

	qDebug() << "UpdateController: launching" << updaterPath << "with args"
			 << args;
	const bool ok = QProcess::startDetached(updaterPath, args);
	if (!ok) {
		qCritical() << "UpdateController: failed to start updater binary.";
		QMessageBox::critical(
			m_parent,
			QCoreApplication::translate("UpdateController", "Update Failed"),
			QCoreApplication::translate("UpdateController",
										"Could not launch the updater "
										"binary.\nPlease update %1 manually.")
				.arg(BuildConfig.MESHMC_DISPLAYNAME));
		return false;
	}

	return true;
}
