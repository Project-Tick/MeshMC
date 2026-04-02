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
#include "TestUtil.h"

#include "settings/INIFile.h"

class IniFileTest : public QObject
{
	Q_OBJECT
  private slots:
	void initTestCase() {}
	void cleanupTestCase() {}

	void test_Escape_data()
	{
		QTest::addColumn<QString>("through");

		QTest::newRow("unix path") << "/abc/def/ghi/jkl";
		QTest::newRow("windows path")
			<< "C:\\Program files\\terrible\\name\\of something\\";
		QTest::newRow("Plain text") << "Lorem ipsum dolor sit amet.";
		QTest::newRow("Escape sequences")
			<< "Lorem\n\t\n\\n\\tAAZ\nipsum dolor\n\nsit amet.";
		QTest::newRow("Escape sequences 2") << "\"\n\n\"";
		QTest::newRow("Hashtags") << "some data#something";
	}
	void test_Escape()
	{
		QFETCH(QString, through);

		QString there = INIFile::escape(through);
		QString back = INIFile::unescape(there);

		QCOMPARE(back, through);
	}

	void test_SaveLoad()
	{
		QString a = "a";
		QString b = "a\nb\t\n\\\\\\C:\\Program files\\terrible\\name\\of "
					"something\\#thisIsNotAComment";
		QString filename = "test_SaveLoad.ini";

		// save
		INIFile f;
		f.set("a", a);
		f.set("b", b);
		f.saveFile(filename);

		// load
		INIFile f2;
		f2.loadFile(filename);
		QCOMPARE(a, f2.get("a", "NOT SET").toString());
		QCOMPARE(b, f2.get("b", "NOT SET").toString());
	}
};

QTEST_GUILESS_MAIN(IniFileTest)

#include "INIFile_test.moc"
