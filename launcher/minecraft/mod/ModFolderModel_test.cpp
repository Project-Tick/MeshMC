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
