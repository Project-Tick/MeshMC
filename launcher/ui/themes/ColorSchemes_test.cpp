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

#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <KColorScheme>
#include <KSharedConfig>

#include "settings/KConfigSettingsObject.h"
#include "ui/themes/ColorSchemes.h"

// The resources are in MeshMC_logic; nothing initialises them in a test.
static void initResources()
{
	Q_INIT_RESOURCE(colorschemes);
}

class ColorSchemesTest : public QObject
{
	Q_OBJECT

	static QByteArray slurp(const QString& path)
	{
		QFile file(path);
		return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
	}

  private slots:
	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
		initResources();
	}

	void test_TheNamesOfEarlierVersionsMapToSchemes_data()
	{
		QTest::addColumn<QString>("theme");
		QTest::addColumn<QString>("id");
		QTest::addColumn<bool>("isNull");
		QTest::newRow("system") << "system" << "" << false;
		QTest::newRow("dark") << "dark" << "BreezeDark" << false;
		QTest::newRow("bright") << "bright" << "BreezeLight" << false;
		QTest::newRow("green dark")
			<< "green-dark" << "MeshMCGreenDark" << false;
		QTest::newRow("green light")
			<< "green-light" << "MeshMCGreenLight" << false;
		// a widget style, or a theme of the user's own, is not a scheme
		QTest::newRow("fusion") << "fusion" << "" << true;
		QTest::newRow("custom") << "mytheme" << "" << true;
	}

	void test_TheNamesOfEarlierVersionsMapToSchemes()
	{
		QFETCH(QString, theme);
		QFETCH(QString, id);
		QFETCH(bool, isNull);
		const QString mapped = ColorSchemes::idForLegacyTheme(theme);
		QCOMPARE(mapped.isNull(), isNull);
		QCOMPARE(mapped, id);
	}

	void test_BundledSchemesAreInstalledOnce()
	{
		QTemporaryDir dir;
		const QString target = dir.filePath("color-schemes");
		QCOMPARE(ColorSchemes::installBundled(target), 2);
		QVERIFY(QFile::exists(target + "/MeshMCGreenDark.colors"));
		QVERIFY(QFile::exists(target + "/MeshMCGreenLight.colors"));

		// nothing to do the second time
		QCOMPARE(ColorSchemes::installBundled(target), 0);

		// an edited file is put back
		{
			QFile file(target + "/MeshMCGreenDark.colors");
			QVERIFY(file.open(QIODevice::WriteOnly));
			file.write("edited");
		}
		QCOMPARE(ColorSchemes::installBundled(target), 1);
		QCOMPARE(slurp(target + "/MeshMCGreenDark.colors"),
				 slurp(":/color-schemes/MeshMCGreenDark.colors"));
	}

	void test_BundledSchemesAreValidKdeSchemes()
	{
		// what the old DarkTheme/Green themes had as their palette
		struct Expected {
			const char* file;
			QColor window;
			QColor view;
			QColor selection;
		};
		const Expected expected[] = {
			{"MeshMCGreenDark", QColor(49, 49, 49), QColor(34, 34, 34),
			 QColor(150, 219, 89)},
			{"MeshMCGreenLight", QColor(255, 255, 255),
			 QColor(250, 250, 250), QColor(137, 207, 84)},
		};
		for (const auto& e : expected) {
			QTemporaryDir dir;
			const QString path = dir.filePath(QString(e.file) + ".colors");
			QVERIFY(QFile::copy(
				QString(":/color-schemes/%1.colors").arg(e.file), path));
			auto config = KSharedConfig::openConfig(path, KConfig::SimpleConfig);

			const KColorScheme window(QPalette::Active, KColorScheme::Window,
									  config);
			const KColorScheme view(QPalette::Active, KColorScheme::View,
									config);
			const KColorScheme selection(QPalette::Active,
										 KColorScheme::Selection, config);
			QCOMPARE(window.background().color(), e.window);
			QCOMPARE(view.background().color(), e.view);
			QCOMPARE(selection.background().color(), e.selection);
		}
	}

	void test_ANativeStyleIsReplacedByFusionForASchemeOfItsOwn()
	{
		// a scheme of the user's on a native style: Fusion
		QCOMPARE(ColorSchemes::styleFor("macos", "macos", true, false),
				 QString("Fusion"));
		QCOMPARE(ColorSchemes::styleFor("windows11", "windows11", true, false),
				 QString("Fusion"));
		// a style that follows the palette is left alone
		QVERIFY(ColorSchemes::styleFor("Fusion", "macos", true, false)
					.isEmpty());
		// the system's scheme: nothing to do, unless the style was replaced
		QVERIFY(ColorSchemes::styleFor("macos", "macos", false, false)
					.isEmpty());
		QCOMPARE(ColorSchemes::styleFor("Fusion", "macos", false, true),
				 QString("macos"));
		// and it is not put back twice, or over a style that was chosen
		QVERIFY(ColorSchemes::styleFor("macos", "macos", false, true)
					.isEmpty());
		QVERIFY(ColorSchemes::styleFor("Fusion", "", false, true).isEmpty());
		QVERIFY(ColorSchemes::isNativeStyle("macOS"));
		QVERIFY(!ColorSchemes::isNativeStyle("Fusion"));
	}

	void test_ASchemeOfTheUsersIsImported()
	{
		QTemporaryDir source;
		QTemporaryDir target;
		const QString file = source.filePath("Mine.colors");
		{
			QFile f(file);
			QVERIFY(f.open(QIODevice::WriteOnly));
			f.write("[General]\nName=Mine\n\n[Colors:Window]\n"
					"BackgroundNormal=1,2,3\n");
		}

		QString error;
		QVERIFY2(ColorSchemes::importScheme(file, target.filePath("schemes"),
											&error),
				 qPrintable(error));
		QCOMPARE(slurp(target.filePath("schemes/Mine.colors")), slurp(file));

		// the same name replaces it
		{
			QFile f(file);
			QVERIFY(f.open(QIODevice::WriteOnly));
			f.write("[Colors:Window]\nBackgroundNormal=9,9,9\n");
		}
		QVERIFY(ColorSchemes::importScheme(file, target.filePath("schemes"),
										   &error));
		QVERIFY(slurp(target.filePath("schemes/Mine.colors"))
					.contains("9,9,9"));
	}

	void test_ThingsThatAreNotSchemesAreNotImported()
	{
		QTemporaryDir source;
		QTemporaryDir target;
		QString error;

		// not a .colors file
		const QString text = source.filePath("notes.txt");
		{
			QFile f(text);
			QVERIFY(f.open(QIODevice::WriteOnly));
			f.write("[Colors:Window]\n");
		}
		QVERIFY(!ColorSchemes::importScheme(text, target.path(), &error));
		QVERIFY(!error.isEmpty());

		// a .colors file without colours
		const QString empty = source.filePath("Empty.colors");
		{
			QFile f(empty);
			QVERIFY(f.open(QIODevice::WriteOnly));
			f.write("[General]\nName=Empty\n");
		}
		error.clear();
		QVERIFY(!ColorSchemes::importScheme(empty, target.path(), &error));
		QVERIFY(!error.isEmpty());

		// nothing at all
		QVERIFY(!ColorSchemes::importScheme(source.filePath("missing.colors"),
											target.path(), &error));
		QVERIFY(!QFile::exists(target.filePath("Empty.colors")));
	}

	void test_TheSettingsOfTheOldThemeSystemAreTakenOnce()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("meshmc.cfg");
		{
			QFile file(path);
			QVERIFY(file.open(QIODevice::WriteOnly));
			file.write("ApplicationTheme=green-dark\nIconTheme=pe_dark\n"
					   "BackgroundCat=rory\nTheCat=true\nCatOpacity=50\n"
					   "InstanceDir=inst\n");
		}
		KConfigSettingsObject settings(path);
		QCOMPARE(ColorSchemes::takeLegacySettings(&settings),
				 QString("green-dark"));

		const QByteArray written = slurp(path);
		QVERIFY(!written.contains("ApplicationTheme"));
		QVERIFY(!written.contains("BackgroundCat"));
		QVERIFY(!written.contains("TheCat"));
		QVERIFY(!written.contains("CatOpacity"));
		// IconTheme is a setting again (AppearanceManager makes an old
		// value Breeze), and what is not ours is left alone
		QVERIFY(written.contains("IconTheme=pe_dark"));
		QVERIFY(written.contains("InstanceDir=inst"));

		QVERIFY(ColorSchemes::takeLegacySettings(&settings).isEmpty());
		QVERIFY(ColorSchemes::takeLegacySettings(nullptr).isEmpty());
	}
};

QTEST_MAIN(ColorSchemesTest)

#include "ColorSchemes_test.moc"
