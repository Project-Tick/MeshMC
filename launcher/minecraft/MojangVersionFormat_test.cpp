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
#include <QDebug>
#include "TestUtil.h"

#include "minecraft/MojangVersionFormat.h"

class MojangVersionFormatTest : public QObject
{
	Q_OBJECT

	static QJsonDocument readJson(const char* file)
	{
		auto path = QFINDTESTDATA(file);
		QFile jsonFile(path);
		if (!jsonFile.open(QIODevice::ReadOnly))
			return QJsonDocument();
		auto data = jsonFile.readAll();
		jsonFile.close();
		return QJsonDocument::fromJson(data);
	}
	static void writeJson(const char* file, QJsonDocument doc)
	{
		QFile jsonFile(file);
		if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
			return;
		auto data = doc.toJson(QJsonDocument::Indented);
		qDebug() << QString::fromUtf8(data);
		jsonFile.write(data);
		jsonFile.close();
	}

  private slots:
	void test_Through_Simple()
	{
		QJsonDocument doc = readJson("data/1.9-simple.json");
		auto vfile =
			MojangVersionFormat::versionFileFromJson(doc, "1.9-simple.json");
		auto doc2 = MojangVersionFormat::versionFileToJson(vfile);
		writeJson("1.9-simple-passthorugh.json", doc2);

		QCOMPARE(doc.toJson(), doc2.toJson());
	}

	void test_Through()
	{
		QJsonDocument doc = readJson("data/1.9.json");
		auto vfile = MojangVersionFormat::versionFileFromJson(doc, "1.9.json");
		auto doc2 = MojangVersionFormat::versionFileToJson(vfile);
		writeJson("1.9-passthorugh.json", doc2);
		QCOMPARE(doc.toJson(), doc2.toJson());
	}
};

QTEST_GUILESS_MAIN(MojangVersionFormatTest)

#include "MojangVersionFormat_test.moc"
