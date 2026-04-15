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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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
#include <Version.h>

class ModUtilsTest : public QObject
{
	Q_OBJECT
	void setupVersions()
	{
		QTest::addColumn<QString>("first");
		QTest::addColumn<QString>("second");
		QTest::addColumn<bool>("lessThan");
		QTest::addColumn<bool>("equal");

		QTest::newRow("equal, explicit") << "1.2.0" << "1.2.0" << false << true;
		QTest::newRow("equal, implicit 1") << "1.2" << "1.2.0" << false << true;
		QTest::newRow("equal, implicit 2") << "1.2.0" << "1.2" << false << true;
		QTest::newRow("equal, two-digit") << "1.42" << "1.42" << false << true;

		QTest::newRow("lessThan, explicit 1")
			<< "1.2.0" << "1.2.1" << true << false;
		QTest::newRow("lessThan, explicit 2")
			<< "1.2.0" << "1.3.0" << true << false;
		QTest::newRow("lessThan, explicit 3")
			<< "1.2.0" << "2.2.0" << true << false;
		QTest::newRow("lessThan, implicit 1")
			<< "1.2" << "1.2.1" << true << false;
		QTest::newRow("lessThan, implicit 2")
			<< "1.2" << "1.3.0" << true << false;
		QTest::newRow("lessThan, implicit 3")
			<< "1.2" << "2.2.0" << true << false;
		QTest::newRow("lessThan, two-digit")
			<< "1.41" << "1.42" << true << false;

		QTest::newRow("greaterThan, explicit 1")
			<< "1.2.1" << "1.2.0" << false << false;
		QTest::newRow("greaterThan, explicit 2")
			<< "1.3.0" << "1.2.0" << false << false;
		QTest::newRow("greaterThan, explicit 3")
			<< "2.2.0" << "1.2.0" << false << false;
		QTest::newRow("greaterThan, implicit 1")
			<< "1.2.1" << "1.2" << false << false;
		QTest::newRow("greaterThan, implicit 2")
			<< "1.3.0" << "1.2" << false << false;
		QTest::newRow("greaterThan, implicit 3")
			<< "2.2.0" << "1.2" << false << false;
		QTest::newRow("greaterThan, two-digit")
			<< "1.42" << "1.41" << false << false;
	}

  private slots:
	void initTestCase() {}
	void cleanupTestCase() {}

	void test_versionCompare_data()
	{
		setupVersions();
	}
	void test_versionCompare()
	{
		QFETCH(QString, first);
		QFETCH(QString, second);
		QFETCH(bool, lessThan);
		QFETCH(bool, equal);

		const auto v1 = Version(first);
		const auto v2 = Version(second);

		QCOMPARE(v1 < v2, lessThan);
		QCOMPARE(v1 > v2, !lessThan && !equal);
		QCOMPARE(v1 == v2, equal);
	}
};

QTEST_GUILESS_MAIN(ModUtilsTest)

#include "Version_test.moc"
