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

#include <QTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "MMCZip.h"
#include "minecraft/mod/Mod.h"

namespace
{
	bool writeFile(const QString& path, const QByteArray& content)
	{
		QFileInfo info(path);
		if (!QDir().mkpath(info.absolutePath())) {
			return false;
		}
		QFile file(path);
		if (!file.open(QIODevice::WriteOnly)) {
			return false;
		}
		return file.write(content) == content.size();
	}

	int countEntries(const QStringList& entries, const QString& name)
	{
		int count = 0;
		for (const auto& entry : entries) {
			if (entry == name) {
				count++;
			}
		}
		return count;
	}
} // namespace

class MMCZipTest : public QObject
{
	Q_OBJECT
  private slots:

	// A zip/jar jar mod replaces game class files, META-INF of the game jar
	// is dropped and no path ends up in the jar twice.
	void test_CreateModdedJar_ZipMod()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		QDir root(tempDir.path());

		auto gameDir = root.absoluteFilePath("game");
		QVERIFY(writeFile(gameDir + "/net/minecraft/Foo.class", "vanilla-foo"));
		QVERIFY(writeFile(gameDir + "/net/minecraft/Bar.class", "vanilla-bar"));
		QVERIFY(writeFile(gameDir + "/META-INF/MANIFEST.MF", "manifest"));
		auto sourceJar = root.absoluteFilePath("minecraft.jar");
		QVERIFY(MMCZip::compressDir(sourceJar, gameDir, nullptr));

		auto modDir = root.absoluteFilePath("modsrc");
		QVERIFY(writeFile(modDir + "/net/minecraft/Foo.class", "modded-foo"));
		auto modZip = root.absoluteFilePath("jarmod.zip");
		QVERIFY(MMCZip::compressDir(modZip, modDir, nullptr));

		QList<Mod> mods;
		mods.append(Mod(QFileInfo(modZip)));
		QCOMPARE(mods[0].type(), Mod::MOD_ZIPFILE);

		auto targetJar = root.absoluteFilePath("modded.jar");
		QVERIFY(MMCZip::createModdedJar(sourceJar, targetJar, mods));

		auto entries = MMCZip::listEntries(targetJar);
		QCOMPARE(countEntries(entries, "net/minecraft/Foo.class"), 1);
		QCOMPARE(countEntries(entries, "net/minecraft/Bar.class"), 1);
		for (const auto& entry : entries) {
			QVERIFY2(!entry.contains("META-INF"),
					 qPrintable("unexpected entry: " + entry));
		}
		QCOMPARE(MMCZip::readFileFromZip(targetJar, "net/minecraft/Foo.class"),
				 QByteArray("modded-foo"));
		QCOMPARE(MMCZip::readFileFromZip(targetJar, "net/minecraft/Bar.class"),
				 QByteArray("vanilla-bar"));
	}

	// A folder jar mod has to be merged into the jar root, otherwise its class
	// files never replace the ones of the game.
	void test_CreateModdedJar_FolderMod()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		QDir root(tempDir.path());

		auto gameDir = root.absoluteFilePath("game");
		QVERIFY(writeFile(gameDir + "/net/minecraft/Foo.class", "vanilla-foo"));
		auto sourceJar = root.absoluteFilePath("minecraft.jar");
		QVERIFY(MMCZip::compressDir(sourceJar, gameDir, nullptr));

		auto modFolder = root.absoluteFilePath("jarmods/mymod");
		QVERIFY(writeFile(modFolder + "/net/minecraft/Foo.class",
						  "folder-modded-foo"));
		QVERIFY(writeFile(modFolder + "/mymod.txt", "hello"));

		QList<Mod> mods;
		mods.append(Mod(QFileInfo(modFolder)));
		QCOMPARE(mods[0].type(), Mod::MOD_FOLDER);

		auto targetJar = root.absoluteFilePath("modded.jar");
		QVERIFY(MMCZip::createModdedJar(sourceJar, targetJar, mods));

		auto entries = MMCZip::listEntries(targetJar);
		QCOMPARE(countEntries(entries, "net/minecraft/Foo.class"), 1);
		QCOMPARE(countEntries(entries, "mymod.txt"), 1);
		QVERIFY2(!entries.contains("mymod/net/minecraft/Foo.class"),
				 "folder jar mods must not be prefixed with the folder name");
		QCOMPARE(MMCZip::readFileFromZip(targetJar, "net/minecraft/Foo.class"),
				 QByteArray("folder-modded-foo"));
	}

	// The jar is only reported as created when it really was written.
	void test_CreateModdedJar_MissingSourceJarFails()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		QDir root(tempDir.path());

		auto modDir = root.absoluteFilePath("modsrc");
		QVERIFY(writeFile(modDir + "/net/minecraft/Foo.class", "modded-foo"));
		auto modZip = root.absoluteFilePath("jarmod.zip");
		QVERIFY(MMCZip::compressDir(modZip, modDir, nullptr));

		QList<Mod> mods;
		mods.append(Mod(QFileInfo(modZip)));

		auto targetJar = root.absoluteFilePath("modded.jar");
		QVERIFY(!MMCZip::createModdedJar(root.absoluteFilePath("nope.jar"),
										 targetJar, mods));
		QVERIFY(!QFile::exists(targetJar));
	}
};

QTEST_GUILESS_MAIN(MMCZipTest)

#include "MMCZip_test.moc"
