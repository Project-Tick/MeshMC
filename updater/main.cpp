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

/*
 * meshmc-updater — standalone updater binary for MeshMC.
 *
 * Usage:
 *   meshmc-updater --url <download_url>
 *                  --root <install_root>
 *                  --exec <relaunch_path>
 *
 *   --url   : Full URL of the artifact to download and install.
 *   --root  : Installation root directory (where meshmc binary lives).
 *   --exec  : Path to the main MeshMC binary to relaunch after update.
 *
 * The binary waits a short time after launch to let the main application
 * exit cleanly before overwriting its files.
 */

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QThread>
#include <QTimer>

#include "Installer.h"

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);
	app.setApplicationName("meshmc-updater");
	app.setOrganizationName("Project Tick");

	QCommandLineParser parser;
	parser.setApplicationDescription("MeshMC Updater");
	parser.addHelpOption();

	QCommandLineOption urlOpt("url", "URL of the update artifact to download.",
							  "url");
	QCommandLineOption rootOpt("root", "Installation root directory.", "root");
	QCommandLineOption execOpt("exec", "Path to relaunch after update.",
							   "exec");

	parser.addOption(urlOpt);
	parser.addOption(rootOpt);
	parser.addOption(execOpt);
	parser.process(app);

	const QString url = parser.value(urlOpt).trimmed();
	const QString root = parser.value(rootOpt).trimmed();
	const QString exec = parser.value(execOpt).trimmed();

	if (url.isEmpty() || root.isEmpty()) {
		fprintf(stderr, "meshmc-updater: --url and --root are required.\n");
		return 1;
	}

	qDebug() << "meshmc-updater: url =" << url << "| root =" << root
			 << "| exec =" << exec;

	// Give the main application 2 seconds to exit before we start overwriting
	// files.
	auto* installer = new Installer(&app);
	installer->setDownloadUrl(url);
	installer->setRootPath(root);
	installer->setRelaunchPath(exec);

	QObject::connect(installer, &Installer::progressMessage,
					 [](const QString& msg) { qDebug() << "updater:" << msg; });

	QObject::connect(
		installer, &Installer::finished,
		[](bool success, const QString& errorMsg) {
			if (!success) {
				qCritical()
					<< "meshmc-updater: installation failed:" << errorMsg;
				QCoreApplication::exit(1);
			} else {
				qDebug() << "meshmc-updater: update installed successfully.";
				QCoreApplication::exit(0);
			}
		});

	// Delay start to give the parent process time to close.
	QTimer::singleShot(2000, &app, [installer]() { installer->start(); });

	return app.exec();
}
