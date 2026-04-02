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

#include <QTest>
#include <QTemporaryDir>
#include "TestUtil.h"

#include "FileSystem.h"
#include "minecraft/mod/ModFolderModel.h"

class ModFolderModelTest : public QObject
{
	Q_OBJECT

  private slots:
	// test for GH-1178 - install a folder with files to a mod list
	void test_1178()
	{
		// source
		QString source = QFINDTESTDATA("data/test_folder");

		// sanity check
		QVERIFY(!source.endsWith('/'));

		auto verify = [](QString path) {
			QDir target_dir(FS::PathCombine(path, "test_folder"));
			QVERIFY(target_dir.entryList().contains("pack.mcmeta"));
			QVERIFY(target_dir.entryList().contains("assets"));
		};

		// 1. test with no trailing /
		{
			QString folder = source;
			QTemporaryDir tempDir;
			ModFolderModel m(tempDir.path());
			m.installMod(folder);
			verify(tempDir.path());
		}

		// 2. test with trailing /
		{
			QString folder = source + '/';
			QTemporaryDir tempDir;
			ModFolderModel m(tempDir.path());
			m.installMod(folder);
			verify(tempDir.path());
		}
	}
};

QTEST_GUILESS_MAIN(ModFolderModelTest)

#include "ModFolderModel_test.moc"
