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
 * meshmc-updater -- installs MeshMC updates.
 *
 *   meshmc-updater --url <url> --root <dir> [--exec <path>]
 *                  [--data-dir <dir>] [--wait-pid <pid>] [--verbose]
 *
 * and, started by the above rather than by a human:
 *
 *   meshmc-updater --apply --source <dir> --root <dir> ...
 *
 * See UpdaterOptions.h for why the work is split across two processes, and
 * UpdaterLog.h for where to look when an update goes wrong.
 */

#include "ApplyStage.h"
#include "PrepareStage.h"
#include "UpdateLock.h"
#include "UpdaterLog.h"
#include "UpdaterOptions.h"

#include "BuildConfig.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>

#include <cstdio>

namespace
{

	enum ExitCode {
		ExitSuccess = 0,
		ExitBadUsage = 2,
		ExitUpdateFailed = 3,
		ExitAlreadyRunning = 4,
	};

	//! Where the log, lock and staging area live when the launcher did not say.
	QString defaultDataDirectory()
	{
		const QString appData =
			QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
		return appData.isEmpty() ? QDir::tempPath() : appData;
	}

	void writeToConsole(const QString& text)
	{
		fputs(qPrintable(text + QLatin1Char('\n')), stderr);
		fflush(stderr);
	}

	/*!
	 * Tell the user, then log it.
	 *
	 * This binary has no console of its own, so without a dialog a failed
	 * update is silent and indistinguishable from a successful one -- which is
	 * precisely how the updater managed to be broken without anybody noticing.
	 */
	void reportFailure(const QString& title, const QString& text)
	{
		qCritical().noquote() << title << "--" << text;

		const QString logPath = UpdaterLog::filePath();
		QMessageBox box;
		box.setWindowTitle(title);
		box.setIcon(QMessageBox::Critical);
		box.setText(text);
		if (!logPath.isEmpty())
			box.setDetailedText(
				QCoreApplication::translate("main", "Full log:\n%1")
					.arg(QDir::toNativeSeparators(logPath)));
		box.setTextInteractionFlags(Qt::TextSelectableByMouse);
		box.setStandardButtons(QMessageBox::Ok);
		box.exec();
	}

	void logHeader(const UpdaterOptions& options)
	{
		qDebug().noquote() << "MeshMC updater"
						   << BuildConfig.printableVersionString() << "("
						   << BuildConfig.GIT_COMMIT << ")";
		qDebug().noquote() << "  stage    :"
						   << (options.stage == UpdaterStage::Apply
								   ? "apply"
								   : "prepare");
		qDebug().noquote() << "  root     :" << options.root;
		qDebug().noquote() << "  data dir :" << options.dataDir;
		qDebug().noquote() << "  exec     :" << options.exec;
		if (options.stage == UpdaterStage::Apply)
			qDebug().noquote() << "  source   :" << options.source;
		else
			qDebug().noquote() << "  url      :" << options.url;
		qDebug().noquote() << "  wait pid :" << options.waitPid;
		qDebug().noquote() << "  running  :"
						   << QCoreApplication::applicationFilePath();
	}

} // namespace

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	app.setOrganizationName(BuildConfig.MESHMC_NAME);
	app.setOrganizationDomain(BuildConfig.MESHMC_DOMAIN);
	app.setApplicationName(BuildConfig.MESHMC_NAME);
	app.setApplicationVersion(BuildConfig.printableVersionString());

	UpdaterOptions options;
	QString parseError;
	const bool parsedCleanly = options.parse(app.arguments(), parseError);

	if (options.verbose || options.helpRequested)
		UpdaterLog::attachToParentConsole();

	if (options.helpRequested) {
		writeToConsole(options.helpText);
		return ExitSuccess;
	}

	if (options.dataDir.isEmpty())
		options.dataDir = defaultDataDirectory();

	// Logging comes up before the first thing that can fail. The second stage
	// appends, so one update reads as one story.
	UpdaterLog::start(options.dataDir, options.stage == UpdaterStage::Prepare,
					  options.verbose);
	logHeader(options);

	if (!parsedCleanly) {
		reportFailure(QCoreApplication::translate("main", "Update Failed"),
					  QCoreApplication::translate(
						  "main", "The updater was started with "
								  "arguments it does not understand.\n\n%1")
						  .arg(parseError));
		UpdaterLog::stop();
		return ExitBadUsage;
	}

	const QString problem = options.validate();
	if (!problem.isEmpty()) {
		reportFailure(QCoreApplication::translate("main", "Update Failed"),
					  problem);
		UpdaterLog::stop();
		return ExitBadUsage;
	}

	UpdateLock lock(options.dataDir);
	if (options.stage == UpdaterStage::Prepare) {
		UpdateLockInfo previous;
		if (!lock.claim(QStringLiteral("prepare"), options.root, options.url,
						previous)) {
			reportFailure(
				QCoreApplication::translate("main", "Update Already Running"),
				QCoreApplication::translate(
					"main",
					"Another MeshMC update is already in progress.\n\n%1\n\n"
					"Wait for it to finish, or close it and try again.")
					.arg(previous.describe()));
			UpdaterLog::stop();
			return ExitAlreadyRunning;
		}
		if (previous.present)
			qWarning().noquote()
				<< "A previous update did not finish:" << previous.describe()
				<< "-- the installation may be a mix of two versions.";
	}

	int exitCode = ExitSuccess;

	const auto reportStageResult = [&](bool ok, const QString& error) {
		if (ok) {
			qDebug() << "Update stage finished successfully.";
		} else {
			exitCode = ExitUpdateFailed;
			reportFailure(QCoreApplication::translate("main", "Update Failed"),
						  error);
		}
		app.quit();
	};

	const auto logProgress = [](const QString& message) {
		qDebug().noquote() << message;
	};

	if (options.stage == UpdaterStage::Prepare) {
		auto* stage = new PrepareStage(options, lock, &app);
		QObject::connect(stage, &PrepareStage::progress, &app, logProgress);
		QObject::connect(stage, &PrepareStage::finished, &app,
						 [&](bool ok, const QString& error, bool handedOff) {
							 if (ok && handedOff)
								 qDebug() << "Handed the update over; the "
											 "second stage takes it from here.";
							 reportStageResult(ok, error);
						 });
		QTimer::singleShot(0, stage, &PrepareStage::start);
	} else {
		auto* stage = new ApplyStage(options, lock, &app);
		QObject::connect(stage, &ApplyStage::progress, &app, logProgress);
		QObject::connect(stage, &ApplyStage::finished, &app, reportStageResult);
		QTimer::singleShot(0, stage, &ApplyStage::start);
	}

	app.exec();

	qDebug() << "meshmc-updater exiting with code" << exitCode;
	UpdaterLog::stop();
	return exitCode;
}
