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

#include "minecraft/GradleSpecifier.h"

class GradleSpecifierTest : public QObject
{
	Q_OBJECT
  private slots:
	void initTestCase() {}
	void cleanupTestCase() {}

	void test_Positive_data()
	{
		QTest::addColumn<QString>("through");

		QTest::newRow("3 parter") << "org.gradle.test.classifiers:service:1.0";
		QTest::newRow("classifier")
			<< "org.gradle.test.classifiers:service:1.0:jdk15";
		QTest::newRow("jarextension")
			<< "org.gradle.test.classifiers:service:1.0@jar";
		QTest::newRow("jarboth")
			<< "org.gradle.test.classifiers:service:1.0:jdk15@jar";
		QTest::newRow("packxz")
			<< "org.gradle.test.classifiers:service:1.0:jdk15@jar.pack.xz";
	}
	void test_Positive()
	{
		QFETCH(QString, through);

		QString converted = GradleSpecifier(through).serialize();

		QCOMPARE(converted, through);
	}

	void test_Path_data()
	{
		QTest::addColumn<QString>("spec");
		QTest::addColumn<QString>("expected");

		QTest::newRow("3 parter") << "group.id:artifact:1.0"
								  << "group/id/artifact/1.0/artifact-1.0.jar";
		QTest::newRow("doom") << "id.software:doom:1.666:demons@wad"
							  << "id/software/doom/1.666/doom-1.666-demons.wad";
	}
	void test_Path()
	{
		QFETCH(QString, spec);
		QFETCH(QString, expected);

		QString converted = GradleSpecifier(spec).toPath();

		QCOMPARE(converted, expected);
	}
	void test_Negative_data()
	{
		QTest::addColumn<QString>("input");

		QTest::newRow("too many :")
			<< "org:gradle.test:class:::ifiers:service:1.0::";
		QTest::newRow("nonsense") << "I like turtles";
		QTest::newRow("empty string") << "";
		QTest::newRow("missing version") << "herp.derp:artifact";
	}
	void test_Negative()
	{
		QFETCH(QString, input);

		GradleSpecifier spec(input);
		QVERIFY(!spec.valid());
		QCOMPARE(spec.serialize(), input);
		QCOMPARE(spec.toPath(), QString());
	}
};

QTEST_GUILESS_MAIN(GradleSpecifierTest)

#include "GradleSpecifier_test.moc"
