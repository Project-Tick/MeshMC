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

#include <QTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "modplatform/PackContents.h"

class PackContentsTest : public QObject
{
	Q_OBJECT
  private slots:

	// The ordinary case, so the tests below are known to be measuring
	// something other than a broken harness.
	void test_RoundTrip()
	{
		QTemporaryDir root;
		QVERIFY(root.isValid());

		QVERIFY(PackContents::write(root.path(),
									{"mods/sodium.jar", "config/sodium.txt"}));

		QStringList read;
		QVERIFY(PackContents::read(root.path(), read));
		QCOMPARE(read, QStringList({"config/sodium.txt", "mods/sodium.jar"}));
	}

	// Stored sorted and de-duplicated, so that the same version installed
	// twice produces the same file and a diff between two versions only
	// shows what really changed.
	void test_WriteIsSortedAndDeduplicated()
	{
		QTemporaryDir root;
		QVERIFY(root.isValid());

		QVERIFY(PackContents::write(
			root.path(), {"mods/b.jar", "mods/a.jar", "mods/b.jar"}));

		QStringList read;
		QVERIFY(PackContents::read(root.path(), read));
		QCOMPARE(read, QStringList({"mods/a.jar", "mods/b.jar"}));
	}

	// An instance installed before the launcher recorded this has no list.
	// That has to be distinguishable from "the version shipped nothing",
	// because the two mean opposite things for a delete loop.
	void test_MissingListIsNotReadable()
	{
		QTemporaryDir root;
		QVERIFY(root.isValid());

		QStringList read{"stays untouched"};
		QVERIFY(!PackContents::read(root.path(), read));
	}

	void test_EmptyListIsReadable()
	{
		QTemporaryDir root;
		QVERIFY(root.isValid());

		QVERIFY(PackContents::write(root.path(), {}));

		QStringList read{"replaced"};
		QVERIFY(PackContents::read(root.path(), read));
		QVERIFY(read.isEmpty());
	}

	// A list written by a future build may mean something else entirely,
	// and guessing wrong deletes files.
	void test_UnknownFormatVersionIsRefused()
	{
		QTemporaryDir root;
		QVERIFY(root.isValid());

		writeRaw(root.path(),
				 R"({"formatVersion":2,"paths":["mods/sodium.jar"]})");

		QStringList read;
		QVERIFY(!PackContents::read(root.path(), read));
	}

	void test_MalformedListIsRefused()
	{
		QTemporaryDir root;
		QVERIFY(root.isValid());

		writeRaw(root.path(), R"({"formatVersion":1,"paths":)");

		QStringList read;
		QVERIFY(!PackContents::read(root.path(), read));
	}

	void test_ListWithoutPathsArrayIsRefused()
	{
		QTemporaryDir root;
		QVERIFY(root.isValid());

		writeRaw(root.path(), R"({"formatVersion":1})");

		QStringList read;
		QVERIFY(!PackContents::read(root.path(), read));
	}

	void test_NormalizePath()
	{
		QCOMPARE(PackContents::normalizePath("mods/sodium.jar"),
				 QStringLiteral("mods/sodium.jar"));
		// Written on Windows, read anywhere.
		QCOMPARE(PackContents::normalizePath("mods\\sodium.jar"),
				 QStringLiteral("mods/sodium.jar"));
		QCOMPARE(PackContents::normalizePath("./mods//sodium.jar"),
				 QStringLiteral("mods/sodium.jar"));
		QCOMPARE(PackContents::normalizePath("config/../mods/sodium.jar"),
				 QStringLiteral("mods/sodium.jar"));

		// Refused rather than repaired: these are fed to a delete loop.
		QVERIFY(PackContents::normalizePath("").isEmpty());
		QVERIFY(PackContents::normalizePath(".").isEmpty());
		QVERIFY(PackContents::normalizePath("../outside.jar").isEmpty());
		QVERIFY(
			PackContents::normalizePath("mods/../../outside.jar").isEmpty());
		QVERIFY(PackContents::normalizePath("/etc/passwd").isEmpty());
	}

	void test_RefusedPathsNeverReachTheList()
	{
		QTemporaryDir root;
		QVERIFY(root.isValid());

		QVERIFY(PackContents::write(
			root.path(), {"mods/ok.jar", "../escape.jar", "/etc/passwd"}));

		QStringList read;
		QVERIFY(PackContents::read(root.path(), read));
		QCOMPARE(read, QStringList({"mods/ok.jar"}));
	}

	// The whole point: a mod the new version dropped is stale, one it
	// still ships is not.
	void test_StaleEntriesFindsDroppedFiles()
	{
		const QStringList stale = PackContents::staleEntries(
			{"mods/kept.jar", "mods/dropped.jar"}, {"mods/kept.jar"});

		QVERIFY(stale.contains("mods/dropped.jar"));
		QVERIFY(!stale.contains("mods/kept.jar"));
	}

	// A file the user put there themselves was never in a list of ours, so
	// it can never come out of one. An update that ate the user's own mods
	// would be worse than one that leaves a stale jar behind.
	void test_StaleEntriesIgnoresUnrecordedFiles()
	{
		const QStringList stale =
			PackContents::staleEntries({"mods/pack.jar"}, {});

		QCOMPARE(stale.count("mods/user-added.jar"), 0);
		QVERIFY(stale.contains("mods/pack.jar"));
	}

	// A mod the user turned off is still a file the pack installed, and it
	// is recorded under the name it had at install time.
	void test_StaleEntriesFollowsDisabledRename()
	{
		const QStringList stale =
			PackContents::staleEntries({"mods/dropped.jar"}, {});

		QVERIFY(stale.contains("mods/dropped.jar"));
		QVERIFY(stale.contains("mods/dropped.jar.disabled"));
	}

	void test_StaleEntriesFollowsEnabledRename()
	{
		const QStringList stale =
			PackContents::staleEntries({"mods/dropped.jar.disabled"}, {});

		QVERIFY(stale.contains("mods/dropped.jar.disabled"));
		QVERIFY(stale.contains("mods/dropped.jar"));
	}

	// An update that turns a required mod into an optional one installs it
	// as ".disabled". Deleting the file the update has just written would
	// be worse than leaving a stale one.
	void test_StaleEntriesNeverTouchesWhatTheUpdateShips()
	{
		const QStringList stale = PackContents::staleEntries(
			{"mods/flipped.jar"}, {"mods/flipped.jar.disabled"});

		QVERIFY(!stale.contains("mods/flipped.jar.disabled"));
		QVERIFY(stale.contains("mods/flipped.jar"));
	}

	void test_StaleEntriesNeverTouchesWhatTheUpdateShipsEnabled()
	{
		const QStringList stale = PackContents::staleEntries(
			{"mods/flipped.jar.disabled"}, {"mods/flipped.jar"});

		QVERIFY(!stale.contains("mods/flipped.jar"));
		QVERIFY(stale.contains("mods/flipped.jar.disabled"));
	}

	// Stripping ".disabled" off a bare suffix would name the containing
	// folder, and the caller deletes what it is given.
	void test_StaleEntriesNeverNamesAFolder()
	{
		const QStringList stale =
			PackContents::staleEntries({"mods/.disabled"}, {});

		QVERIFY(stale.contains("mods/.disabled"));
		QVERIFY(!stale.contains("mods"));
		QVERIFY(!stale.contains("mods/"));
	}

	// Identical lists mean an update that changed no files - a re-install
	// of the same version, say. Nothing may be deleted.
	void test_StaleEntriesOfIdenticalListsIsEmpty()
	{
		const QStringList files{"mods/a.jar", "config/a.txt"};
		QVERIFY(PackContents::staleEntries(files, files).isEmpty());
	}

	// Written on Windows, diffed on Linux: the same file must not look
	// like two different ones.
	void test_StaleEntriesComparesNormalizedPaths()
	{
		QVERIFY(PackContents::staleEntries({"mods\\a.jar"}, {"mods/a.jar"})
					.isEmpty());
	}

  private:
	static void writeRaw(const QString& instanceRoot, const char* contents)
	{
		QFile file(PackContents::listPath(instanceRoot));
		QVERIFY(file.open(QIODevice::WriteOnly));
		QVERIFY(file.write(contents) > 0);
		file.close();
	}
};

QTEST_GUILESS_MAIN(PackContentsTest)

#include "PackContents_test.moc"
