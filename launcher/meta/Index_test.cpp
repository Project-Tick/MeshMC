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
