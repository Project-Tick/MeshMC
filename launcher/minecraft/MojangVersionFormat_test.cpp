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
