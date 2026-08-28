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

#include "UpdaterOptions_test.h"

#include "UpdaterOptions.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

namespace
{

	//! An installation root with an application binary sitting in it.
	struct FakeInstall {
		QTemporaryDir dir;
		QString root;
		QString exec;

		FakeInstall()
		{
			root = QDir::cleanPath(dir.path());
			exec = QDir(root).absoluteFilePath("meshmc.exe");
			QFile binary(exec);
			binary.open(QIODevice::WriteOnly);
			binary.write("not really a binary");
			binary.close();
		}
	};

	QStringList commandLine(const QStringList& arguments)
	{
		return QStringList{"meshmc-updater"} + arguments;
	}

} // namespace

void UpdaterOptionsTest::tst_Unquote_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QString>("expected");

	QTest::newRow("plain") << "C:/dir"
						   << "C:/dir";
	QTest::newRow("double quoted") << "\"C:/dir\""
								   << "C:/dir";
	QTest::newRow("single quoted") << "'C:/dir'"
								   << "C:/dir";
	// cmd.exe does not treat ' as a quote, so this is what actually arrives.
	QTest::newRow("single quoted with trailing separator") << "'C:\\dir\\'"
														   << "C:\\dir\\";
	// In `--root "C:\dir\"` the backslash escapes the closing quote and the
	// argument arrives with one quote too many.
	QTest::newRow("unmatched trailing quote") << "C:\\dir\""
											  << "C:\\dir";
	QTest::newRow("both kinds") << "\"'C:/dir'\""
								<< "C:/dir";
	QTest::newRow("surrounding whitespace") << "  'C:/dir'  "
											<< "C:/dir";
	QTest::newRow("empty") << ""
						   << "";
	QTest::newRow("only quotes") << "''"
								 << "";
}

void UpdaterOptionsTest::tst_Unquote()
{
	QFETCH(QString, input);
	QFETCH(QString, expected);
	QCOMPARE(UpdaterOptions::unquote(input), expected);
}

void UpdaterOptionsTest::tst_CleanPathArgument_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QString>("expected");

	const QString base = QDir::cleanPath(QDir::tempPath());

	QTest::newRow("already clean") << base << base;
	QTest::newRow("trailing slash") << base + "/" << base;
	QTest::newRow("quoted") << "'" + base + "'" << base;
	QTest::newRow("quoted with trailing slash") << "'" + base + "/'" << base;
	QTest::newRow("redundant components") << base + "/./sub/.." << base;
	QTest::newRow("empty") << ""
						   << "";
}

void UpdaterOptionsTest::tst_CleanPathArgument()
{
	QFETCH(QString, input);
	QFETCH(QString, expected);
	QCOMPARE(UpdaterOptions::cleanPathArgument(input), expected);
}

void UpdaterOptionsTest::tst_QuotedRootFromCmdShell()
{
	// The command line from the original bug report, pointed at a directory
	// that exists here. Before the fix, root kept its apostrophes, no path
	// built from it could resolve, and the updater died without a word.
	//
	// The trailing separator has to be the platform's own: a backslash is a
	// path separator on Windows but an ordinary filename character on POSIX,
	// where cleanPath() rightly keeps it.
	FakeInstall install;
	const QString quotedRoot = QLatin1Char('\'') +
							   QDir::toNativeSeparators(install.root) +
							   QDir::separator() + QLatin1Char('\'');

	UpdaterOptions options;
	QString error;
	QVERIFY(options.parse(
		commandLine({"--url", "https://example.invalid/u.zip", "--root",
					 quotedRoot, "--exec", "'" + install.exec + "'"}),
		error));

	QVERIFY2(!options.root.contains('\''),
			 qPrintable("root is still quoted: " + options.root));
	QCOMPARE(options.root, install.root);
	QCOMPARE(options.exec, install.exec);
	QCOMPARE(options.validate(), QString());
}

void UpdaterOptionsTest::tst_QuotedRootWithBackslashOnWindows()
{
#ifdef Q_OS_WIN
	// Literally what cmd.exe delivered in the bug report: single quotes it
	// never treated as quotes, around a path ending in a backslash.
	FakeInstall install;

	UpdaterOptions options;
	QString error;
	QVERIFY(options.parse(commandLine({"--url", "https://example.invalid/u.zip",
									   "--root", "'" + install.root + "\\'"}),
						  error));
	QCOMPARE(options.root, install.root);
	QCOMPARE(options.validate(), QString());
#else
	QSKIP("cmd.exe quoting is a Windows-only problem");
#endif
}

void UpdaterOptionsTest::tst_ParseMinimalPrepare()
{
	UpdaterOptions options;
	QString error;
	QVERIFY(options.parse(commandLine({"--url", "https://example.invalid/u.zip",
									   "--root", QDir::tempPath()}),
						  error));

	QVERIFY(error.isEmpty());
	QCOMPARE(options.stage, UpdaterStage::Prepare);
	QCOMPARE(options.url, QStringLiteral("https://example.invalid/u.zip"));
	QCOMPARE(options.waitPid, 0);
	QVERIFY(!options.verbose);
	QVERIFY(!options.helpRequested);
}

void UpdaterOptionsTest::tst_ParseApplyStage()
{
	UpdaterOptions options;
	QString error;
	QVERIFY(options.parse(
		commandLine({"--apply", "--source", QDir::tempPath(), "--root",
					 QDir::tempPath(), "--wait-pid", "4242", "--verbose"}),
		error));

	QCOMPARE(options.stage, UpdaterStage::Apply);
	QCOMPARE(options.waitPid, 4242);
	QVERIFY(options.verbose);
}

void UpdaterOptionsTest::tst_ParseRejectsUnknownOption()
{
	UpdaterOptions options;
	QString error;
	QVERIFY(!options.parse(commandLine({"--reticulate-splines"}), error));
	QVERIFY(!error.isEmpty());
}

void UpdaterOptionsTest::tst_ParseRejectsNonNumericWaitPid()
{
	UpdaterOptions options;
	QString error;
	QVERIFY(!options.parse(
		commandLine({"--root", QDir::tempPath(), "--url",
					 "https://example.invalid/u.zip", "--wait-pid", "soon"}),
		error));
	QVERIFY(error.contains("wait-pid"));
}

void UpdaterOptionsTest::tst_ParseHelp()
{
	UpdaterOptions options;
	QString error;
	QVERIFY(options.parse(commandLine({"--help"}), error));
	QVERIFY(options.helpRequested);
	QVERIFY(options.helpText.contains("--apply"));
	QVERIFY(options.helpText.contains("--wait-pid"));
}

void UpdaterOptionsTest::tst_ValidateRejectsMissingRoot()
{
	UpdaterOptions options;
	options.url = "https://example.invalid/u.zip";
	QVERIFY(options.validate().contains("--root"));
}

void UpdaterOptionsTest::tst_ValidateRejectsNonExistentRoot()
{
	UpdaterOptions options;
	options.url = "https://example.invalid/u.zip";
	options.root =
		QDir(QDir::tempPath()).absoluteFilePath("meshmc-no-such-directory");
	QVERIFY(options.validate().contains("does not exist"));
}

void UpdaterOptionsTest::tst_ValidateRejectsRootThatIsAFile()
{
	FakeInstall install;
	UpdaterOptions options;
	options.url = "https://example.invalid/u.zip";
	options.root = install.exec; // a file, not a directory
	QVERIFY(options.validate().contains("not a directory"));
}

void UpdaterOptionsTest::tst_ValidateRejectsMissingUrl()
{
	FakeInstall install;
	UpdaterOptions options;
	options.root = install.root;
	QVERIFY(options.validate().contains("--url"));
}

void UpdaterOptionsTest::tst_ValidateRejectsNonHttpUrl()
{
	FakeInstall install;
	UpdaterOptions options;
	options.root = install.root;
	options.url = "file:///etc/passwd";
	QVERIFY(!options.validate().isEmpty());
}

void UpdaterOptionsTest::tst_ValidateRejectsMissingExec()
{
	FakeInstall install;
	UpdaterOptions options;
	options.root = install.root;
	options.url = "https://example.invalid/u.zip";
	options.exec = QDir(install.root).absoluteFilePath("not-here.exe");
	QVERIFY(options.validate().contains("does not exist"));
}

void UpdaterOptionsTest::tst_ValidateAcceptsAGoodPrepare()
{
	FakeInstall install;
	UpdaterOptions options;
	options.root = install.root;
	options.exec = install.exec;
	options.url = "https://example.invalid/u.zip";
	QCOMPARE(options.validate(), QString());
}

void UpdaterOptionsTest::tst_ValidateRejectsApplyWithoutSource()
{
	FakeInstall install;
	UpdaterOptions options;
	options.stage = UpdaterStage::Apply;
	options.root = install.root;
	QVERIFY(options.validate().contains("--source"));
}

void UpdaterOptionsTest::tst_ValidateAcceptsAGoodApply()
{
	FakeInstall install;
	QTemporaryDir staging;

	UpdaterOptions options;
	options.stage = UpdaterStage::Apply;
	options.root = install.root;
	options.source = QDir::cleanPath(staging.path());
	// The binary is about to be replaced, so Apply must not insist that the
	// one named by --exec is still where it was.
	options.exec = QDir(install.root).absoluteFilePath("gone.exe");
	QCOMPARE(options.validate(), QString());
}

void UpdaterOptionsTest::tst_ApplyArgumentsRoundTrip()
{
	FakeInstall install;
	QTemporaryDir staging;
	QTemporaryDir data;

	UpdaterOptions first;
	first.root = install.root;
	first.exec = install.exec;
	first.dataDir = QDir::cleanPath(data.path());
	first.url = "https://example.invalid/u.zip";
	first.verbose = true;

	const QString stagingPath = QDir::cleanPath(staging.path());
	UpdaterOptions second;
	QString error;
	QVERIFY(second.parse(commandLine(first.applyArguments(stagingPath, 1234)),
						 error));

	QCOMPARE(second.stage, UpdaterStage::Apply);
	QCOMPARE(second.source, stagingPath);
	QCOMPARE(second.root, first.root);
	QCOMPARE(second.exec, first.exec);
	QCOMPARE(second.dataDir, first.dataDir);
	QCOMPARE(second.waitPid, 1234);
	QCOMPARE(second.verbose, true);
	QCOMPARE(second.validate(), QString());
}

QTEST_GUILESS_MAIN(UpdaterOptionsTest)
