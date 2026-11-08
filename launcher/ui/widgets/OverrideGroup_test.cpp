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

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "settings/KConfigSettingsObject.h"
#include "settings/OverrideSetting.h"
#include "settings/Setting.h"
#include "ui/widgets/OverrideGroup.h"

// What the page of an instance relies on: a global setting, the instance's
// override of it, and the gate that says which one is read.
class OverrideGroupTest : public QObject
{
	Q_OBJECT

	struct Fixture {
		QTemporaryDir dir;
		KConfigSettingsObject global;
		KConfigSettingsObject instance;
		Fixture()
			: global(dir.filePath("meshmc.cfg")),
			  instance(dir.filePath("instance.cfg"))
		{
			global.registerSetting("MinMemAlloc", 512);
			global.registerSetting("MaxMemAlloc", 1024);
			global.registerSetting("JvmArgs", QString("-Dglobal"));
			global.registerSetting("JavaAutoDownloadVendor",
								   QString("vendor.a"));
			auto memory = instance.registerSetting("OverrideMemory", false);
			instance.registerOverride(global.getSetting("MinMemAlloc"), memory);
			instance.registerOverride(global.getSetting("MaxMemAlloc"), memory);
			auto javaOverride = instance.registerSetting("OverrideJava", false);
			auto argsOverride =
				instance.registerSetting("OverrideJavaArgs", false);
			auto argsGate = argsOverride;
			Q_UNUSED(javaOverride);
			instance.registerOverride(global.getSetting("JvmArgs"), argsGate);
			auto vendor = instance.registerSetting(
				"OverrideJavaAutoDownloadVendor", false);
			instance.registerOverride(global.getSetting("JavaAutoDownloadVendor"),
									  vendor);
			instance.registerSetting("JoinServerOnLaunch", false);
			instance.registerSetting("JoinServerOnLaunchAddress", QString());
		}
	};

	static QByteArray slurp(const QString& path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

  private slots:
	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
	}

	void test_GateOffShowsTheGlobalValues()
	{
		Fixture f;
		f.global.set("MinMemAlloc", 700);

		QGroupBox box;
		box.setCheckable(true);
		QSpinBox min, max;
		min.setMaximum(100000);
		max.setMaximum(100000);
		OverrideGroup group(&f.instance, &box, "OverrideMemory");
		group.bind("MinMemAlloc", &min).bind("MaxMemAlloc", &max);

		group.load();
		QVERIFY(!box.isChecked());
		QCOMPARE(min.value(), 700);
		QCOMPARE(max.value(), 1024);
	}

	void test_GateOnWritesTheInstanceAndNotTheGlobal()
	{
		Fixture f;
		QGroupBox box;
		box.setCheckable(true);
		QSpinBox min, max;
		min.setMaximum(100000);
		max.setMaximum(100000);
		OverrideGroup group(&f.instance, &box, "OverrideMemory");
		group.bind("MinMemAlloc", &min).bind("MaxMemAlloc", &max);
		group.load();

		box.setChecked(true);
		min.setValue(2048);
		max.setValue(4096);
		group.apply();

		QCOMPARE(f.instance.get("OverrideMemory").toBool(), true);
		QCOMPARE(f.instance.get("MinMemAlloc").toInt(), 2048);
		QCOMPARE(f.instance.get("MaxMemAlloc").toInt(), 4096);
		// the global setting is as it was
		QCOMPARE(f.global.get("MinMemAlloc").toInt(), 512);
		QVERIFY(slurp(f.dir.filePath("instance.cfg")).contains("MinMemAlloc=2048"));
	}

	void test_GateOffAgainResetsWhatWasWritten()
	{
		Fixture f;
		QGroupBox box;
		box.setCheckable(true);
		QSpinBox min;
		min.setMaximum(100000);
		OverrideGroup group(&f.instance, &box, "OverrideMemory");
		group.bind("MinMemAlloc", &min);
		group.load();

		box.setChecked(true);
		min.setValue(2048);
		group.apply();
		QCOMPARE(f.instance.get("MinMemAlloc").toInt(), 2048);

		box.setChecked(false);
		group.apply();
		QCOMPARE(f.instance.get("OverrideMemory").toBool(), false);
		// follows the global setting again
		QCOMPARE(f.instance.get("MinMemAlloc").toInt(), 512);
		QVERIFY(!slurp(f.dir.filePath("instance.cfg")).contains("MinMemAlloc"));
	}

	void test_AnOlderGateStillSwitchesTheGroupOn()
	{
		Fixture f;
		// what an earlier version wrote: one gate for the location and the
		// arguments
		f.instance.set("OverrideJava", true);

		QGroupBox box;
		box.setCheckable(true);
		QPlainTextEdit args;
		OverrideGroup group(&f.instance, &box, "OverrideJavaArgs");
		group.legacyGate("OverrideJava").bind("JvmArgs", &args);
		group.load();
		QVERIFY(box.isChecked());

		// and is dropped when the page is applied
		group.apply();
		QCOMPARE(f.instance.get("OverrideJava").toBool(), false);
		QCOMPARE(f.instance.get("OverrideJavaArgs").toBool(), true);
	}

	void test_AValueIsChangedOnTheWayOut()
	{
		Fixture f;
		QGroupBox box;
		box.setCheckable(true);
		QPlainTextEdit args;
		OverrideGroup group(&f.instance, &box, "OverrideJavaArgs");
		group.bind("JvmArgs", &args).transformOnApply("JvmArgs", [](QVariant v) {
			return QVariant(v.toString().replace("\n", " "));
		});
		group.load();
		box.setChecked(true);
		args.setPlainText("-Xa\n-Xb");
		group.apply();
		QCOMPARE(f.instance.get("JvmArgs").toString(), QString("-Xa -Xb"));
	}

	void test_AComboBoxIsBoundByItsData()
	{
		Fixture f;
		QGroupBox box;
		box.setCheckable(true);
		QComboBox combo;
		combo.addItem("Vendor A", "vendor.a");
		combo.addItem("Vendor B", "vendor.b");
		OverrideGroup group(&f.instance, &box, "OverrideJavaAutoDownloadVendor");
		group.bind("JavaAutoDownloadVendor", &combo);
		group.load();
		QCOMPARE(combo.currentData().toString(), QString("vendor.a"));

		box.setChecked(true);
		combo.setCurrentIndex(1);
		group.apply();
		QCOMPARE(f.instance.get("JavaAutoDownloadVendor").toString(),
				 QString("vendor.b"));

		// a vendor that is not in the list leaves the choice alone
		f.instance.set("JavaAutoDownloadVendor", QString("vendor.gone"));
		combo.setCurrentIndex(0);
		group.load();
		QCOMPARE(combo.currentData().toString(), QString("vendor.a"));
	}

	void test_TheGateCanBeTheSettingItself()
	{
		// JoinServerOnLaunch has no global setting: the gate is the value
		Fixture f;
		QGroupBox box;
		box.setCheckable(true);
		QLineEdit address;
		OverrideGroup group(&f.instance, &box, "JoinServerOnLaunch");
		group.bind("JoinServerOnLaunchAddress", &address);
		group.load();
		QVERIFY(!box.isChecked());

		box.setChecked(true);
		address.setText("play.example.org");
		group.apply();
		QCOMPARE(f.instance.get("JoinServerOnLaunch").toBool(), true);
		QCOMPARE(f.instance.get("JoinServerOnLaunchAddress").toString(),
				 QString("play.example.org"));

		box.setChecked(false);
		group.apply();
		QCOMPARE(f.instance.get("JoinServerOnLaunch").toBool(), false);
		QVERIFY(f.instance.get("JoinServerOnLaunchAddress").toString().isEmpty());
	}

	void test_AGateThatIsNotAGroupBox()
	{
		Fixture f;
		bool on = false;
		QSpinBox min;
		min.setMaximum(100000);
		OverrideGroup group(
			&f.instance, "OverrideMemory", [&on] { return on; },
			[&on](bool v) { on = v; });
		group.bind("MinMemAlloc", &min);
		f.instance.set("OverrideMemory", true);
		f.instance.set("MinMemAlloc", 900);
		group.load();
		QVERIFY(on);
		QCOMPARE(min.value(), 900);
	}
};

QTEST_MAIN(OverrideGroupTest)

#include "OverrideGroup_test.moc"
