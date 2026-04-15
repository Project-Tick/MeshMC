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
#include "TestUtil.h"

#include "meta/Index.h"
#include "meta/VersionList.h"

class IndexTest : public QObject
{
	Q_OBJECT
  private slots:
	void test_hasUid_and_getList()
	{
		Meta::Index windex({std::make_shared<Meta::VersionList>("list1"),
							std::make_shared<Meta::VersionList>("list2"),
							std::make_shared<Meta::VersionList>("list3")});
		QVERIFY(windex.hasUid("list1"));
		QVERIFY(!windex.hasUid("asdf"));
		QVERIFY(windex.get("list2") != nullptr);
		QCOMPARE(windex.get("list2")->uid(), QString("list2"));
		QVERIFY(windex.get("adsf") != nullptr);
	}

	void test_merge()
	{
		Meta::Index windex({std::make_shared<Meta::VersionList>("list1"),
							std::make_shared<Meta::VersionList>("list2"),
							std::make_shared<Meta::VersionList>("list3")});
		QCOMPARE(windex.lists().size(), 3);
		windex.merge(std::shared_ptr<Meta::Index>(
			new Meta::Index({std::make_shared<Meta::VersionList>("list1"),
							 std::make_shared<Meta::VersionList>("list2"),
							 std::make_shared<Meta::VersionList>("list3")})));
		QCOMPARE(windex.lists().size(), 3);
		windex.merge(std::shared_ptr<Meta::Index>(
			new Meta::Index({std::make_shared<Meta::VersionList>("list4"),
							 std::make_shared<Meta::VersionList>("list2"),
							 std::make_shared<Meta::VersionList>("list5")})));
		QCOMPARE(windex.lists().size(), 5);
		windex.merge(std::shared_ptr<Meta::Index>(
			new Meta::Index({std::make_shared<Meta::VersionList>("list6")})));
		QCOMPARE(windex.lists().size(), 6);
	}
};

QTEST_GUILESS_MAIN(IndexTest)

#include "Index_test.moc"
