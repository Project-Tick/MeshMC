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

#include <QGuiApplication>
#include <QSet>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <KConfigGroup>
#include <KCoreConfigSkeleton>

#include "settings/GlobalSettings.h"
#include "settings/GlobalSettingsSetup.h"
#include "settings/KConfigSettingsObject.h"
#include "settings/Setting.h"

// The global settings are declared in GlobalSettings.kcfg and registered from
// the class kconfig_compiler makes of it. These are the things the rest of the
// program relies on, written down once.
class GlobalSettingsSetupTest : public QObject
{
	Q_OBJECT

	struct Fixture {
		QTemporaryDir dir;
		KConfigSettingsObject settings;
		Fixture() : settings(dir.filePath("meshmc.cfg"))
		{
			registerGlobalSettings(&settings);
		}
	};

  private slots:
	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
	}

	void test_EverySettingIsThere()
	{
		Fixture f;
		// 81 when the .kcfg was written; a setting that is added brings this
		// up, one that is lost would make a caller's get() return nothing.
		QVERIFY(f.settings.allSettings().size() >= 81);
		for (const char* id :
			 {"InstanceDir", "JavaPath", "ProxyType", "MinMemAlloc",
			  "ApplicationTheme", "CurseForgeAPIKey", "PasteEEAPIKey",
			  "MacNativeMenuBar", "PagedGeometry", "JProfilerPath",
			  "JProfilerPort", "JVisualVMPath", "MCEditPath"}) {
			QVERIFY2(f.settings.getSetting(id), id);
		}
	}

	void test_DefaultsAndTheirTypes()
	{
		Fixture f;
		QCOMPARE(f.settings.get("InstanceDir").toString(),
				 QString("instances"));
		QCOMPARE(f.settings.get("ProxyPort").toInt(), 8080);
		QCOMPARE(f.settings.get("MinecraftWinWidth").toInt(), 854);
		QCOMPARE(f.settings.get("JavaAutoDownload").toBool(), true);
		QCOMPARE(f.settings.get("PasteEEAPIKey").toString(),
				 QString("meshmc"));
		// the type of the default is what the settings dialog binds a
		// widget to
		QCOMPARE(f.settings.getSetting("ShowConsole")->defValue().metaType().id(),
				 int(QMetaType::Bool));
		QCOMPARE(f.settings.getSetting("ConsoleMaxLines")
					 ->defValue()
					 .metaType()
					 .id(),
				 int(QMetaType::Int));
		QCOMPARE(f.settings.getSetting("JavaTimestamp")
					 ->defValue()
					 .metaType()
					 .id(),
				 int(QMetaType::LongLong));
	}

	void test_TheNamesEarlierVersionsUsedStillRead_data()
	{
		QTest::addColumn<QString>("id");
		QTest::addColumn<QString>("older");
		QTest::newRow("window width") << "MinecraftWinWidth" << "MCWindowWidth";
		QTest::newRow("maximized") << "LaunchMaximized" << "MCWindowMaximize";
		QTest::newRow("proxy host") << "ProxyAddr" << "ProxyHostName";
		QTest::newRow("min memory") << "MinMemAlloc" << "MinMemoryAlloc";
		QTest::newRow("pre-launch") << "PreLaunchCommand" << "PreLaunchCmd";
		QTest::newRow("mods dir") << "CentralModsDir" << "ModsDir";
		QTest::newRow("backup plugin")
			<< "BackupBeforeLaunch" << "plugin.backup_system.BackupBeforeLaunch";
		QTest::newRow("menu bar") << "MacNativeMenuBar" << "UseMacNativeMenuBar";
	}

	void test_TheNamesEarlierVersionsUsedStillRead()
	{
		QFETCH(QString, id);
		QFETCH(QString, older);
		Fixture f;
		const auto keys = f.settings.getSetting(id)->configKeys();
		QCOMPARE(keys.first(), id);
		QVERIFY2(keys.contains(older), qPrintable(older));
	}

	void test_AValueUnderAnOlderNameIsFound()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("meshmc.cfg");
		{
			QFile file(path);
			QVERIFY(file.open(QIODevice::WriteOnly));
			file.write("MCWindowWidth=1234\nProxyHostName=10.0.0.1\n");
		}
		KConfigSettingsObject settings(path);
		registerGlobalSettings(&settings);
		QCOMPARE(settings.get("MinecraftWinWidth").toInt(), 1234);
		QCOMPARE(settings.get("ProxyAddr").toString(), QString("10.0.0.1"));
	}

	void test_TypedAccessorsAndGetSeeTheSameValues()
	{
		Fixture f;
		auto* typed = typedGlobalSettings(&f.settings);
		QCOMPARE(typed->proxyPort(), 8080);

		// a set() is seen by the accessor...
		f.settings.set("ProxyPort", 9999);
		QCOMPARE(typed->proxyPort(), 9999);
		f.settings.set("ProxyAddr", QString("10.1.1.1"));
		QCOMPARE(typed->proxyAddr(), QString("10.1.1.1"));
		f.settings.reset("ProxyPort");
		QCOMPARE(typed->proxyPort(), 8080);

		// ...and a setter that is saved is seen by get()
		typed->setProxyPort(7777);
		typed->save();
		QCOMPARE(f.settings.get("ProxyPort").toInt(), 7777);
	}

	void test_ASavedSetterTellsTheListenersOfTheSetting()
	{
		// the windows, the themes and the wizard write through the setters
		Fixture f;
		auto* typed = typedGlobalSettings(&f.settings);
		int changed = 0;
		QVariant seen;
		connect(f.settings.getSetting("MainWindowGeometry").get(),
				&Setting::SettingChanged, this,
				[&](const Setting&, const QVariant& value) {
					++changed;
					seen = value;
				});
		typed->setMainWindowGeometry(QStringLiteral("AAEC"));
		typed->setSelectedInstance(QStringLiteral("abc"));
		typed->save();
		QCOMPARE(changed, 1);
		QCOMPARE(seen.toString(), QString("AAEC"));
		QCOMPARE(f.settings.get("MainWindowGeometry").toString(),
				 QString("AAEC"));
		QCOMPARE(f.settings.get("SelectedInstance").toString(),
				 QString("abc"));
	}

	void test_AnItemSetToItsDefaultLeavesTheFile()
	{
		// JavaWizardPage: without a PermGen the key is removed
		Fixture f;
		auto* typed = typedGlobalSettings(&f.settings);
		typed->setProxyPort(7777);
		typed->save();
		QVERIFY(f.settings.config()
					->group(QString())
					.hasKey(QStringLiteral("ProxyPort")));

		typed->proxyPortItem()->setDefault();
		typed->save();
		QVERIFY(!f.settings.config()
					 ->group(QString())
					 .hasKey(QStringLiteral("ProxyPort")));
		QCOMPARE(f.settings.get("ProxyPort").toInt(), 8080);
		QCOMPARE(typed->proxyPort(), 8080);
	}

	void test_TheDialogSkeletonHasAnItemForEverySetting()
	{
		Fixture f;
		auto* skeleton = f.settings.skeleton();
		QSet<QString> items;
		for (auto* item : skeleton->items()) {
			items.insert(item->name());
		}
		for (const auto& setting : f.settings.allSettings()) {
			QVERIFY2(items.contains(setting->id()), qPrintable(setting->id()));
		}
	}
};

QTEST_MAIN(GlobalSettingsSetupTest)

#include "GlobalSettingsSetup_test.moc"
