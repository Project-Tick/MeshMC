#include <QTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <KCoreConfigSkeleton>

#include "settings/KConfigSettingsObject.h"
#include "settings/Setting.h"

namespace
{
	QByteArray slurp(const QString& path)
	{
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly)) {
			return QByteArray();
		}
		return file.readAll();
	}

	void spit(const QString& path, const QByteArray& data)
	{
		QFile file(path);
		QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(path));
		file.write(data);
	}
} // namespace

class KConfigSettingsObjectTest : public QObject
{
	Q_OBJECT
  private slots:
	void initTestCase()
	{
		// KConfig has to stay out of the real configuration directories, even
		// if one of these tests ever hands it a relative path.
		QStandardPaths::setTestModeEnabled(true);
	}

	void test_RoundTrip_data()
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
		QTest::newRow("Backslash and hash") << "a\\#b";
		QTest::newRow("Colour") << "#ff8800";
		QTest::newRow("JVM arguments")
			<< "-Xmx2G -XX:+UseG1GC -Dfoo=bar;baz";
		QTest::newRow("Equals signs") << "k=v=w";
		QTest::newRow("Looks like a section") << "[not a group]";
		QTest::newRow("Unicode") << QString::fromUtf8("çiçek 漢字 ✓");
		QTest::newRow("Empty") << "";
	}
	void test_RoundTrip()
	{
		QFETCH(QString, through);

		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const QString path = dir.filePath("test.cfg");

		{
			KConfigSettingsObject settings(path);
			settings.registerSetting("Value", QString("default"));
			settings.set("Value", through);
		}

		KConfigSettingsObject again(path);
		again.registerSetting("Value", QString("default"));
		QCOMPARE(again.get("Value").toString(), through);
	}

	void test_Default()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("test.cfg"));
		settings.registerSetting("Value", QString("default"));
		QCOMPARE(settings.get("Value").toString(), QString("default"));
	}

	void test_Synonyms()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");
		spit(path, "Old=1\n");

		KConfigSettingsObject settings(path);
		settings.registerSetting({"New", "Old"}, QString("default"));
		// the old name is still understood
		QCOMPARE(settings.get("New").toString(), QString("1"));

		// writing uses the first name and drops the other
		settings.set("New", QString("2"));
		const QByteArray written = slurp(path);
		QVERIFY(written.contains("New=2"));
		QVERIFY(!written.contains("Old"));
	}

	void test_Reset()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");

		{
			KConfigSettingsObject settings(path);
			settings.registerSetting("Value", QString("default"));
			settings.registerSetting("Other", QString("other"));
			settings.set("Value", QString("changed"));
			settings.set("Other", QString("kept"));
			settings.reset("Value");
		}

		KConfigSettingsObject again(path);
		again.registerSetting("Value", QString("default"));
		again.registerSetting("Other", QString("other"));
		QCOMPARE(again.get("Value").toString(), QString("default"));
		QCOMPARE(again.get("Other").toString(), QString("kept"));
	}

	void test_ResettingEverythingKeepsTheFile()
	{
		// InstanceList decides that a folder is an instance by looking for its
		// instance.cfg, so the file must not vanish when it runs empty.
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");

		KConfigSettingsObject settings(path);
		settings.registerSetting("Value", QString("default"));
		settings.set("Value", QString("changed"));
		QVERIFY(QFile::exists(path));
		settings.reset("Value");
		QVERIFY(QFile::exists(path));
	}

	void test_LegacyEscapes()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");
		// What older versions wrote: '#' as "\#", a backslash as "\\", and
		// the value  a\#b  as  a\\\#b.
		const QByteArray legacy = "Plain=hello\n"
								  "Hash=some data\\#something\n"
								  "BackslashHash=a\\\\\\#b\n"
								  "Lines=l1\\nl2\\tx\n"
								  "Path=C:\\\\dir\\\\\n";
		spit(path, legacy);

		{
			KConfigSettingsObject settings(path);
			for (const auto& key :
				 {"Plain", "Hash", "BackslashHash", "Lines", "Path"}) {
				settings.registerSetting(key, QString("default"));
			}
			QCOMPARE(settings.get("Plain").toString(), QString("hello"));
			QCOMPARE(settings.get("Hash").toString(),
					 QString("some data#something"));
			QCOMPARE(settings.get("BackslashHash").toString(),
					 QString("a\\#b"));
			QCOMPARE(settings.get("Lines").toString(), QString("l1\nl2\tx"));
			QCOMPARE(settings.get("Path").toString(), QString("C:\\dir\\"));
		}

		// the original is kept
		QCOMPARE(slurp(path + ".pre-kconfig"), legacy);

		// opening it again changes nothing, and the backup stays the original
		const QByteArray converted = slurp(path);
		QVERIFY(converted != legacy);
		{
			KConfigSettingsObject settings(path);
			settings.registerSetting("Hash", QString("default"));
			QCOMPARE(settings.get("Hash").toString(),
					 QString("some data#something"));
		}
		QCOMPARE(slurp(path), converted);
		QCOMPARE(slurp(path + ".pre-kconfig"), legacy);
	}

	void test_NoBackupWhenNothingToConvert()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");
		{
			KConfigSettingsObject settings(path);
			settings.registerSetting("Value", QString());
			settings.set("Value", QString("a#b\\#c"));
		}
		{
			KConfigSettingsObject settings(path);
			settings.registerSetting("Value", QString());
			QCOMPARE(settings.get("Value").toString(), QString("a#b\\#c"));
		}
		QVERIFY(!QFile::exists(path + ".pre-kconfig"));
	}

	void test_ForeignGeneralGroup()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("instance.cfg");
		// Prism Launcher's layout
		spit(path, "[General]\nInstanceType=OneSix\nname=Pack\n");

		KConfigSettingsObject settings(path);
		settings.registerSetting("InstanceType", QString());
		settings.registerSetting("name", QString("default"));
		QCOMPARE(settings.get("InstanceType").toString(), QString("OneSix"));
		QCOMPARE(settings.get("name").toString(), QString("Pack"));

		// a new value replaces the one in [General] ...
		settings.set("name", QString("Renamed"));
		QCOMPARE(settings.get("name").toString(), QString("Renamed"));
		// ... and after a reset the old one must not come back
		settings.reset("name");
		QCOMPARE(settings.get("name").toString(), QString("default"));
	}

	void test_SuspendSave()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");

		KConfigSettingsObject settings(path);
		settings.registerSetting("A", QString());
		settings.registerSetting("B", QString());
		settings.suspendSave();
		settings.set("A", QString("1"));
		settings.set("B", QString("2"));
		QVERIFY(!QFile::exists(path));
		settings.resumeSave();
		QVERIFY(QFile::exists(path));

		const QByteArray written = slurp(path);
		QVERIFY(written.contains("A=1"));
		QVERIFY(written.contains("B=2"));
	}

	void test_RelativePathIsRelativeToWorkingDirectory()
	{
		QTemporaryDir dir;
		const QString before = QDir::currentPath();
		QVERIFY(QDir::setCurrent(dir.path()));

		{
			KConfigSettingsObject settings("relative.cfg");
			settings.registerSetting("Value", QString());
			settings.set("Value", QString("here"));
			// compared as canonical paths: the temporary folder may be reached
			// through a symbolic link (/var -> /private/var on macOS)
			QCOMPARE(QFileInfo(settings.filePath()).canonicalFilePath(),
					 QFileInfo(dir.filePath("relative.cfg")).canonicalFilePath());
		}
		QDir::setCurrent(before);

		// KConfig on its own would have put it into the configuration folder
		QVERIFY(QFile::exists(dir.filePath("relative.cfg")));
	}

	void test_CreatesMissingFolder()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("not/there/yet/test.cfg");

		KConfigSettingsObject settings(path);
		settings.registerSetting("Value", QString());
		settings.set("Value", QString("x"));
		QVERIFY(QFile::exists(path));
	}

	void test_Reload()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");

		KConfigSettingsObject settings(path);
		settings.registerSetting("Value", QString("default"));
		settings.set("Value", QString("1"));

		spit(path, "Value=2\n");
		QVERIFY(settings.reload());
		QCOMPARE(settings.get("Value").toString(), QString("2"));
	}

	void test_WriteKeepsExternalChangesToOtherKeys()
	{
		// KConfig merges what it writes into the file as it is on disk, so a
		// stale in-memory copy does not undo a change somebody else made to a
		// key this object did not touch.
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");

		KConfigSettingsObject settings(path);
		settings.registerSetting("A", QString("default"));
		settings.registerSetting("B", QString("default"));
		settings.set("A", QString("1"));
		settings.set("B", QString("1"));

		spit(path, "A=changed-outside\nB=1\n");
		settings.set("B", QString("2"));

		// looked at in the file: another object for the same path would share
		// this one's KConfig, and with it what it has in memory
		const QByteArray written = slurp(path);
		QVERIFY(written.contains("B=2"));
		QVERIFY(written.contains("A=changed-outside"));
	}

	void test_WriteDoesNotRefreshTheObjectItself()
	{
		// ... but the object keeps answering from its own copy until it is
		// asked to reload(); this is why InstanceList reloads after an update.
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");

		KConfigSettingsObject settings(path);
		settings.registerSetting("A", QString("default"));
		settings.set("A", QString("1"));

		spit(path, "A=changed-outside\n");
		QCOMPARE(settings.get("A").toString(), QString("1"));
		QVERIFY(settings.reload());
		QCOMPARE(settings.get("A").toString(), QString("changed-outside"));
	}

	void test_ReloadWithoutFile()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("missing.cfg"));
		settings.registerSetting("Value", QString("default"));
		QVERIFY(!settings.reload());
		QCOMPARE(settings.get("Value").toString(), QString("default"));
	}

	void test_Skeleton_TypedItems()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");
		spit(path, "Flag=true\nNumber=42\nBig=5000000000\nRatio=0.5\n"
				   "Text=hello\n");

		KConfigSettingsObject settings(path);
		settings.registerSetting("Flag", false);
		settings.registerSetting("Number", 7);
		settings.registerSetting("Big", qlonglong(1));
		settings.registerSetting("Ratio", 1.0);
		settings.registerSetting("Text", QString("default"));
		settings.registerSetting("Missing", QString("fallback"));
		settings.registerSetting("MissingNumber", 11);

		auto* skeleton = settings.skeleton();
		QVERIFY(skeleton);
		QCOMPARE(skeleton->findItem("Flag")->property(), QVariant(true));
		QCOMPARE(skeleton->findItem("Number")->property(), QVariant(42));
		QCOMPARE(skeleton->findItem("Big")->property().toLongLong(),
				 5000000000LL);
		QCOMPARE(skeleton->findItem("Ratio")->property().toDouble(), 0.5);
		QCOMPARE(skeleton->findItem("Text")->property().toString(),
				 QString("hello"));
		// not in the file: the default of the setting
		QCOMPARE(skeleton->findItem("Missing")->property().toString(),
				 QString("fallback"));
		QCOMPARE(skeleton->findItem("MissingNumber")->property(), QVariant(11));
		QVERIFY(!skeleton->findItem("Nothing"));
	}

	void test_Skeleton_SaveWritesFileAndReportsChanges()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");

		KConfigSettingsObject settings(path);
		auto flag = settings.registerSetting("Flag", false);
		auto number = settings.registerSetting("Number", 7);
		auto untouched = settings.registerSetting("Untouched", QString("x"));

		auto* skeleton = settings.skeleton();
		int flagChanged = 0, numberChanged = 0, untouchedChanged = 0;
		connect(flag.get(), &Setting::SettingChanged, this,
				[&flagChanged] { ++flagChanged; });
		connect(number.get(), &Setting::SettingChanged, this,
				[&numberChanged] { ++numberChanged; });
		connect(untouched.get(), &Setting::SettingChanged, this,
				[&untouchedChanged] { ++untouchedChanged; });

		skeleton->findItem("Flag")->setProperty(true);
		skeleton->findItem("Number")->setProperty(9);
		QVERIFY(skeleton->save());

		// the settings see it, like after a set()
		QCOMPARE(settings.get("Flag").toBool(), true);
		QCOMPARE(settings.get("Number").toInt(), 9);
		QCOMPARE(flagChanged, 1);
		QCOMPARE(numberChanged, 1);
		QCOMPARE(untouchedChanged, 0);

		const QByteArray written = slurp(path);
		QVERIFY(written.contains("Flag=true"));
		QVERIFY(written.contains("Number=9"));
		QVERIFY(!written.contains("Untouched"));
	}

	void test_Skeleton_SeesWhatSetWrote()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("test.cfg"));
		settings.registerSetting("Number", 7);
		settings.registerSetting("Text", QString("default"));

		settings.set("Number", 12);
		settings.set("Text", QString("changed"));

		// built after the set(): the items start from what is in the file
		auto* skeleton = settings.skeleton();
		QCOMPARE(skeleton->findItem("Number")->property(), QVariant(12));
		QCOMPARE(skeleton->findItem("Text")->property().toString(),
				 QString("changed"));
	}

	void test_Skeleton_OlderNameIsMovedToTheCurrentOne()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");
		spit(path, "Old=kept\n");

		KConfigSettingsObject settings(path);
		settings.registerSetting({"New", "Old"}, QString("default"));
		QCOMPARE(settings.skeleton()->findItem("New")->property().toString(),
				 QString("kept"));
		QCOMPARE(settings.get("New").toString(), QString("kept"));
	}

	void test_Skeleton_HasNoItemsForOverrides()
	{
		QTemporaryDir dir;
		KConfigSettingsObject global(dir.filePath("global.cfg"));
		global.registerSetting("JavaPath", QString("java"));

		auto instance = std::make_shared<KConfigSettingsObject>(
			dir.filePath("instance.cfg"));
		auto gate = instance->registerSetting("OverrideJava", false);
		instance->registerOverride(global.getSetting("JavaPath"), gate);

		QVERIFY(instance->skeleton()->findItem("OverrideJava"));
		QVERIFY(!instance->skeleton()->findItem("JavaPath"));
		QVERIFY(global.skeleton()->findItem("JavaPath"));
	}

	void test_AdoptedSkeleton_IsTheOneTheDialogGets()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("test.cfg"));
		settings.registerSetting("Number", 7);

		auto adopted = std::make_unique<KCoreConfigSkeleton>(settings.config());
		// the settings are at the top level of the file, which is the group
		// kconfig_compiler calls "<default>"
		adopted->setCurrentGroup(QStringLiteral("<default>"));
		static qint32 number = 7;
		adopted->addItemInt("Number", number, 7);
		auto* raw = adopted.get();
		settings.adoptSkeleton(std::move(adopted));

		QCOMPARE(settings.skeleton(), raw);
	}

	void test_AdoptedSkeleton_SeesASet()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("test.cfg"));
		settings.registerSetting("Number", 7);

		auto adopted = std::make_unique<KCoreConfigSkeleton>(settings.config());
		// the settings are at the top level of the file, which is the group
		// kconfig_compiler calls "<default>"
		adopted->setCurrentGroup(QStringLiteral("<default>"));
		static qint32 number = 7;
		adopted->addItemInt("Number", number, 7);
		settings.adoptSkeleton(std::move(adopted));
		QCOMPARE(number, 7);

		// what a set() writes is what an item that was made earlier shows
		settings.set("Number", 12);
		QCOMPARE(number, 12);
		settings.reset("Number");
		QCOMPARE(number, 7);
	}

	void test_AdoptedSkeleton_ItsSaveIsASet()
	{
		QTemporaryDir dir;
		KConfigSettingsObject settings(dir.filePath("test.cfg"));
		auto setting = settings.registerSetting("Number", 7);

		auto adopted = std::make_unique<KCoreConfigSkeleton>(settings.config());
		// the settings are at the top level of the file, which is the group
		// kconfig_compiler calls "<default>"
		adopted->setCurrentGroup(QStringLiteral("<default>"));
		static qint32 number = 7;
		adopted->addItemInt("Number", number, 7);
		auto* raw = adopted.get();
		settings.adoptSkeleton(std::move(adopted));

		int changed = 0;
		connect(setting.get(), &Setting::SettingChanged, this,
				[&changed] { ++changed; });
		number = 31;
		raw->save();
		QCOMPARE(settings.get("Number").toInt(), 31);
		QCOMPARE(changed, 1);
	}

	void test_Skeleton_ReloadReadsTheFileAgain()
	{
		QTemporaryDir dir;
		const QString path = dir.filePath("test.cfg");
		KConfigSettingsObject settings(path);
		settings.registerSetting("Text", QString("default"));
		settings.set("Text", QString("one"));
		auto* skeleton = settings.skeleton();
		QCOMPARE(skeleton->findItem("Text")->property().toString(),
				 QString("one"));

		spit(path, "Text=two\n");
		QVERIFY(settings.reload());
		QCOMPARE(skeleton->findItem("Text")->property().toString(),
				 QString("two"));
	}
};

QTEST_GUILESS_MAIN(KConfigSettingsObjectTest)

#include "KConfigSettingsObject_test.moc"
