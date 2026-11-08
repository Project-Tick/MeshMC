/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "settings/KConfigSettingsObject.h"
#include "translations/Translations.h"

class TranslationsTest : public QObject
{
	Q_OBJECT

	// What KSwitchLanguageDialog reads and writes: [Language]/<name of the
	// executable>, as a QByteArray.
	static QByteArray chosen(const QString& file)
	{
		QSettings overrides(file, QSettings::IniFormat);
		overrides.beginGroup(QStringLiteral("Language"));
		return overrides.value(qAppName()).toByteArray();
	}

  private slots:
	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
	}

	void test_KeysOfEarlierVersions_data()
	{
		QTest::addColumn<QString>("key");
		QTest::addColumn<QString>("expected");
		QTest::newRow("english") << "en_GB" << "en";
		QTest::newRow("portuguese") << "pt_PT" << "pt";
		QTest::newRow("kanji") << "ja_KANJI" << "ja";
		QTest::newRow("unchanged") << "de" << "de";
		QTest::newRow("with country") << "es_UY" << "es_UY";
	}

	void test_KeysOfEarlierVersions()
	{
		QFETCH(QString, key);
		QFETCH(QString, expected);
		QCOMPARE(Translations::currentLanguageKey(key), expected);
	}

	void test_TheSettingMovesToTheDialogsFile()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("meshmc.cfg"));
		settings.registerSetting("Language", QString());
		settings.set("Language", QString("de"));
		const QString file = dir.filePath("klanguageoverridesrc");

		QCOMPARE(Translations::migrateLegacyLanguage(&settings, file),
				 QString("de"));
		// where the dialog looks for it
		QCOMPARE(chosen(file), QByteArray("de"));
		// and nowhere else
		QVERIFY(settings.get("Language").toString().isEmpty());
		QVERIFY(!QFile::exists(dir.filePath("meshmc.cfg")) ||
				!QFile(dir.filePath("meshmc.cfg"))
					 .readAll()
					 .contains("Language"));

		// once only
		QCOMPARE(Translations::migrateLegacyLanguage(&settings, file),
				 QString());
		QCOMPARE(chosen(file), QByteArray("de"));
	}

	void test_AnOldKeyIsMovedAsTheLanguageThatStandsForIt()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("meshmc.cfg"));
		settings.registerSetting("Language", QString());
		settings.set("Language", QString("ja_KANJI"));
		const QString file = dir.filePath("klanguageoverridesrc");

		QCOMPARE(Translations::migrateLegacyLanguage(&settings, file),
				 QString("ja"));
		QCOMPARE(chosen(file), QByteArray("ja"));
	}

	void test_TheChoiceOfTheDialogIsNotReplaced()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("meshmc.cfg"));
		settings.registerSetting("Language", QString());
		settings.set("Language", QString("de"));
		const QString file = dir.filePath("klanguageoverridesrc");
		{
			// what the dialog writes: a list, a language and the ones to
			// fall back to
			QSettings overrides(file, QSettings::IniFormat);
			overrides.beginGroup(QStringLiteral("Language"));
			overrides.setValue(qAppName(), QByteArray("fr:en_US"));
		}

		QCOMPARE(Translations::migrateLegacyLanguage(&settings, file),
				 QString());
		QCOMPARE(chosen(file), QByteArray("fr:en_US"));
		// the old setting is dropped all the same, it is not the choice
		QVERIFY(settings.get("Language").toString().isEmpty());
	}

	void test_NoSettingNoFile()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("meshmc.cfg"));
		settings.registerSetting("Language", QString());
		const QString file = dir.filePath("klanguageoverridesrc");

		QCOMPARE(Translations::migrateLegacyLanguage(&settings, file),
				 QString());
		QVERIFY(!QFile::exists(file));
	}

	void test_OtherApplicationsKeepTheirLanguage()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("meshmc.cfg"));
		settings.registerSetting("Language", QString());
		settings.set("Language", QString("de"));
		const QString file = dir.filePath("klanguageoverridesrc");
		{
			QSettings overrides(file, QSettings::IniFormat);
			overrides.beginGroup(QStringLiteral("Language"));
			overrides.setValue(QStringLiteral("dolphin"), QByteArray("sv"));
		}

		QCOMPARE(Translations::migrateLegacyLanguage(&settings, file),
				 QString("de"));
		QSettings overrides(file, QSettings::IniFormat);
		overrides.beginGroup(QStringLiteral("Language"));
		QCOMPARE(overrides.value(QStringLiteral("dolphin")).toByteArray(),
				 QByteArray("sv"));
	}

	void test_FrameworkQtCatalogsAreInTheOrderOfPreference()
	{
		QTemporaryDir dir;
		for (const char* path :
			 {"de/LC_MESSAGES/a_qt.qm", "de/LC_MESSAGES/b_qt.qm",
			  "tr/LC_MESSAGES/a_qt.qm", "fr/LC_MESSAGES/a_qt.qm"}) {
			const QString full = dir.filePath(QString::fromLatin1(path));
			QVERIFY(QDir().mkpath(QFileInfo(full).absolutePath()));
			QFile file(full);
			QVERIFY(file.open(QIODevice::WriteOnly));
		}

		// German is preferred to Turkish; there is no Spanish; French is not
		// asked for. The one that is preferred is installed last.
		const QStringList found = Translations::frameworkQtCatalogs(
			dir.path(), {"de", "es", "tr", "de"},
			{"a_qt", "b_qt", "c_qt"});
		QCOMPARE(found,
				 (QStringList{dir.filePath("tr/LC_MESSAGES/a_qt.qm"),
							  dir.filePath("de/LC_MESSAGES/b_qt.qm"),
							  dir.filePath("de/LC_MESSAGES/a_qt.qm")}));
	}

	void test_NothingIsRegisteredWhereThereIsNothing()
	{
		QTemporaryDir dir;
		QCOMPARE(Translations::registerFrameworkCatalogs(dir.path(), {"de"}),
				 0);
	}

	void test_TheFrameworkDomainsComeFromTheBuild()
	{
		// the top-level CMakeLists decides them and installs the catalogs
		QVERIFY(Translations::frameworkGettextDomains().contains("kxmlgui6"));
		QVERIFY(
			Translations::frameworkQtDomains().contains("kcoreaddons6_qt"));
	}
};

QTEST_MAIN(TranslationsTest)

#include "Translations_test.moc"
