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

#include "UpdaterOptions.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

// Option descriptions below are deliberately untranslated: they only ever
// surface in --help, on a console, in front of somebody debugging an update.
// Everything that can reach the error dialog goes through translate().

static bool isQuoteChar(QChar c)
{
	return c == QLatin1Char('"') || c == QLatin1Char('\'');
}

QString UpdaterOptions::unquote(const QString& value)
{
	QString out = value.trimmed();
	while (!out.isEmpty() && isQuoteChar(out.front()))
		out.remove(0, 1);
	while (!out.isEmpty() && isQuoteChar(out.back()))
		out.chop(1);
	return out.trimmed();
}

QString UpdaterOptions::cleanPathArgument(const QString& value)
{
	const QString unquoted = unquote(value);
	if (unquoted.isEmpty())
		return QString();

	// cleanPath() normalises the separators and drops the trailing one, which
	// matters because every path we derive is of the form root + "/name".
	return QDir::cleanPath(QFileInfo(unquoted).absoluteFilePath());
}

bool UpdaterOptions::parse(const QStringList& arguments, QString& error)
{
	QCommandLineParser parser;
	parser.setApplicationDescription(
		"Downloads and installs a MeshMC update. Normally started by MeshMC "
		"itself; the second stage is started by the first.");
	parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

	const QCommandLineOption applyOpt(
		"apply", "Run the second stage: install the already unpacked update in "
				 "--source over --root.");
	const QCommandLineOption urlOpt("url", "Artifact to download.", "url");
	const QCommandLineOption sourceOpt(
		"source", "Unpacked update to install from (--apply only).",
		"directory");
	const QCommandLineOption rootOpt("root", "Installation root to update.",
									 "directory");
	const QCommandLineOption execOpt(
		"exec", "Binary to start once the update is installed.", "path");
	const QCommandLineOption dataOpt(
		"data-dir",
		"Directory holding the updater log, lock file and staging area. "
		"Defaults to the MeshMC data directory.",
		"directory");
	const QCommandLineOption waitOpt(
		"wait-pid", "Wait for this process to exit before replacing any file.",
		"pid");
	const QCommandLineOption verboseOpt(
		"verbose", "Also write the log to the console that started us.");

	parser.addOption(applyOpt);
	parser.addOption(urlOpt);
	parser.addOption(sourceOpt);
	parser.addOption(rootOpt);
	parser.addOption(execOpt);
	parser.addOption(dataOpt);
	parser.addOption(waitOpt);
	parser.addOption(verboseOpt);
	const QCommandLineOption helpOpt = parser.addHelpOption();

	// parse(), not process(): process() writes to a stream this GUI-subsystem
	// binary does not have and then calls ::exit(). That is precisely the
	// "nothing happened and the prompt came straight back" failure we are
	// getting rid of.
	const bool parsedCleanly = parser.parse(arguments);
	helpText = parser.helpText();
	helpRequested = parser.isSet(helpOpt);
	if (!parsedCleanly) {
		error = parser.errorText();
		return false;
	}
	if (helpRequested)
		return true;

	stage =
		parser.isSet(applyOpt) ? UpdaterStage::Apply : UpdaterStage::Prepare;
	url = unquote(parser.value(urlOpt));
	source = cleanPathArgument(parser.value(sourceOpt));
	root = cleanPathArgument(parser.value(rootOpt));
	exec = cleanPathArgument(parser.value(execOpt));
	dataDir = cleanPathArgument(parser.value(dataOpt));
	verbose = parser.isSet(verboseOpt);

	if (parser.isSet(waitOpt)) {
		bool pidOk = false;
		const QString raw = unquote(parser.value(waitOpt));
		waitPid = raw.toLongLong(&pidOk);
		if (!pidOk || waitPid < 0) {
			error = QCoreApplication::translate(
						"UpdaterOptions",
						"--wait-pid expects a process id, got \"%1\".")
						.arg(raw);
			return false;
		}
	}

	return true;
}

QString UpdaterOptions::validate() const
{
	if (root.isEmpty())
		return QCoreApplication::translate(
			"UpdaterOptions", "No installation root was given (--root).");

	const QFileInfo rootInfo(root);
	if (!rootInfo.exists())
		return QCoreApplication::translate(
				   "UpdaterOptions",
				   "The installation root does not exist:\n%1")
			.arg(root);
	if (!rootInfo.isDir())
		return QCoreApplication::translate(
				   "UpdaterOptions",
				   "The installation root is not a directory:\n%1")
			.arg(root);
	if (!rootInfo.isWritable())
		return QCoreApplication::translate(
				   "UpdaterOptions",
				   "The installation root is not writable:\n%1\n\nMeshMC "
				   "cannot update itself without write access to its own "
				   "installation.")
			.arg(root);

	if (stage == UpdaterStage::Prepare) {
		if (url.isEmpty())
			return QCoreApplication::translate(
				"UpdaterOptions", "No download URL was given (--url).");

		const QUrl parsed(url, QUrl::StrictMode);
		if (!parsed.isValid() || parsed.host().isEmpty())
			return QCoreApplication::translate(
					   "UpdaterOptions",
					   "The download URL is not a valid URL:\n%1")
				.arg(url);

		const QString scheme = parsed.scheme().toLower();
		if (scheme != QLatin1String("http") && scheme != QLatin1String("https"))
			return QCoreApplication::translate(
					   "UpdaterOptions",
					   "Refusing to download an update over \"%1\". Only http "
					   "and https are supported.")
				.arg(parsed.scheme());

		if (!exec.isEmpty() && !QFileInfo(exec).isFile())
			return QCoreApplication::translate(
					   "UpdaterOptions",
					   "The binary to restart does not exist:\n%1")
				.arg(exec);
	} else {
		if (source.isEmpty())
			return QCoreApplication::translate(
				"UpdaterOptions", "No unpacked update was given (--source).");
		if (!QFileInfo(source).isDir())
			return QCoreApplication::translate(
					   "UpdaterOptions", "The unpacked update is missing:\n%1")
				.arg(source);
	}

	return QString();
}

QStringList UpdaterOptions::applyArguments(const QString& stagingDir,
										   qint64 prepareStagePid) const
{
	QStringList args{"--apply", "--source",	  stagingDir, "--root",
					 root,		"--data-dir", dataDir};
	if (!exec.isEmpty())
		args << "--exec" << exec;
	if (prepareStagePid > 0)
		args << "--wait-pid" << QString::number(prepareStagePid);
	if (verbose)
		args << "--verbose";
	return args;
}
