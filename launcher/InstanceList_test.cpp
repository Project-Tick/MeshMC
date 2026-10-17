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

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include "InstanceList.h"
#include "NullInstance.h"
#include "modplatform/PackArtwork.h"
#include "settings/INISettingsObject.h"
#include "tasks/Task.h"
#include "tasks/TaskWatcher.h"

namespace
{
bool writeFile(const QString& path, const QByteArray& contents,
			   const QDateTime& modified)
{
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		return false;
	}
	if (file.write(contents) != contents.size()) {
		return false;
	}
	// Flushed before the explicit mtime is set, same as
	// ScreenshotListModel_test.cpp's helper - a write landing after
	// setFileTime() would bump the mtime back to "now".
	if (!file.flush()) {
		return false;
	}
	const bool timeSet =
		file.setFileTime(modified, QFileDevice::FileModificationTime);
	file.close();
	return timeSet;
}

/* Same set of global settings models/InstanceDetails_test.cpp's
 * makeGlobalSettings() registers - BaseInstance's constructor (run for
 * every concrete instance InstanceList::loadInstance() creates) overrides
 * or passes through exactly these ids, and a globalSettings without one of
 * them makes registration hand back a null Setting. */
SettingsObjectPtr makeGlobalSettings(QTemporaryDir& dir)
{
	auto settings =
		std::make_shared<INISettingsObject>(dir.filePath("global.ini"));
	settings->registerSetting("PreLaunchCommand", "");
	settings->registerSetting("WrapperCommand", "");
	settings->registerSetting("PostExitCommand", "");
	settings->registerSetting("ShowConsole", true);
	settings->registerSetting("AutoCloseConsole", false);
	settings->registerSetting("ShowConsoleOnError", true);
	settings->registerSetting("LogPrePostOutput", true);
	settings->registerSetting("ConsoleMaxLines", 100000);
	settings->registerSetting("ConsoleOverflowStop", true);
	return settings;
}

/* A catalogue reply, written with single quotes so it reads as JSON here
 * without a backslash before every quote (and without raw string
 * literals, which moc cannot read past). */
QByteArray reply(const char* json)
{
	return QByteArray(json).replace('\'', '"');
}

/* A Task that only ends when told to, standing in for an install. */
class ScriptedTask : public Task
{
  public:
	using Task::Task;
	void driveSuccess()
	{
		emitSucceeded();
	}

  protected:
	void executeTask() override {}
};
} // namespace

/*
 * The first few tests exercise InstanceList::newestScreenshotUrl() - the
 * lookup CoverImageRole schedules a background scan for in data() - directly,
 * rather than through a full InstanceList: that needs a SettingsObjectPtr and
 * real BaseInstance subclasses to construct, which would make those tests
 * about instance bookkeeping InstanceList already has other coverage for,
 * not about the screenshot lookup itself. HasCrashedRole has no such
 * standalone helper - the flag lives on BaseInstance - and CoverImageRole's
 * own async scheduling/caching/invalidation needs a real row to ask data()
 * about, so those tests build the minimal real InstanceList+NullInstance
 * fixture instead (same fixture shape as models/InstanceDetails_test.cpp's),
 * waiting out the background scan with QTRY_COMPARE.
 */
class InstanceListTest : public QObject
{
	Q_OBJECT
  private slots:

	void picksNewestModifiedImage()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		const QString dir = tempDir.path();

		const QDateTime base = QDateTime::currentDateTime();
		QVERIFY(writeFile(QDir(dir).filePath("older.png"), "a",
						   base.addSecs(-60)));
		const QString newerPath = QDir(dir).filePath("newer.jpg");
		QVERIFY(writeFile(newerPath, "bb", base));

		QCOMPARE(InstanceList::newestScreenshotUrl(dir),
				 QUrl::fromLocalFile(QFileInfo(newerPath).absoluteFilePath())
					 .toString());
	}

	void ignoresNonImageFiles()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		const QString dir = tempDir.path();

		QVERIFY(writeFile(QDir(dir).filePath("notes.txt"), "c",
						   QDateTime::currentDateTime()));

		QCOMPARE(InstanceList::newestScreenshotUrl(dir), QString());
	}

	void matchesExtensionsCaseInsensitively()
	{
		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		const QString dir = tempDir.path();

		const QString path = QDir(dir).filePath("shot.JPEG");
		QVERIFY(writeFile(path, "x", QDateTime::currentDateTime()));

		QCOMPARE(
			InstanceList::newestScreenshotUrl(dir),
			QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath()).toString());
	}

	void emptyForMissingOrEmptyDirectory()
	{
		QCOMPARE(InstanceList::newestScreenshotUrl(QString()), QString());

		QTemporaryDir tempDir;
		QVERIFY(tempDir.isValid());
		QCOMPARE(InstanceList::newestScreenshotUrl(
					 QDir(tempDir.path()).filePath("does-not-exist")),
				 QString());
		QCOMPARE(InstanceList::newestScreenshotUrl(tempDir.path()), QString());
	}

	void hasCrashedRoleReflectsInstanceState()
	{
		QTemporaryDir globalDir;
		QVERIFY(globalDir.isValid());
		QTemporaryDir instsDir;
		QVERIFY(instsDir.isValid());
		SettingsObjectPtr globalSettings = makeGlobalSettings(globalDir);

		const QString instRoot = QDir(instsDir.path()).filePath("testinst");
		QVERIFY(QDir().mkpath(instRoot));
		/* Unrecognized InstanceType, same as a folder InstanceList itself
		 * cannot make sense of: loadInstance() falls back to NullInstance
		 * for anything that is not OneSix/Nostalgia/Legacy, which is the
		 * one concrete BaseInstance simple enough to construct here. */
		QVERIFY(writeFile(QDir(instRoot).filePath("instance.cfg"),
						   "InstanceType=NullTest\n",
						   QDateTime::currentDateTime()));

		InstanceList list(globalSettings, {instsDir.path()});
		QCOMPARE(list.loadList(), InstanceList::NoError);

		InstancePtr inst = list.getInstanceById("testinst");
		QVERIFY(inst);
		const QModelIndex idx = list.getInstanceIndexById("testinst");
		QVERIFY(idx.isValid());

		QCOMPARE(list.roleNames().value(InstanceList::HasCrashedRole),
				 QByteArray("hasCrashed"));
		QCOMPARE(list.data(idx, InstanceList::HasCrashedRole).toBool(),
				 false);

		QSignalSpy dataChangedSpy(&list, &InstanceList::dataChanged);
		inst->setCrashed(true);
		QVERIFY(!dataChangedSpy.isEmpty());
		QCOMPARE(list.data(idx, InstanceList::HasCrashedRole).toBool(), true);

		dataChangedSpy.clear();
		inst->setCrashed(false);
		QVERIFY(!dataChangedSpy.isEmpty());
		QCOMPARE(list.data(idx, InstanceList::HasCrashedRole).toBool(),
				 false);
	}

	/* PackArtRole: empty for an instance not installed from a platform; for
	 * a platform pack whose icon is not an icon file (no LauncherContext
	 * here, so no icon list at all), the pack's upstream logo URL. */
	void packArtRoleOnlyForPlatformPacks()
	{
		QTemporaryDir globalDir;
		QVERIFY(globalDir.isValid());
		QTemporaryDir instsDir;
		QVERIFY(instsDir.isValid());
		SettingsObjectPtr globalSettings = makeGlobalSettings(globalDir);

		const QString plainRoot = QDir(instsDir.path()).filePath("plain");
		const QString packRoot = QDir(instsDir.path()).filePath("pack");
		QVERIFY(QDir().mkpath(plainRoot));
		QVERIFY(QDir().mkpath(packRoot));
		QVERIFY(writeFile(QDir(plainRoot).filePath("instance.cfg"),
						   "InstanceType=NullTest\n",
						   QDateTime::currentDateTime()));
		QVERIFY(writeFile(QDir(packRoot).filePath("instance.cfg"),
						   "InstanceType=NullTest\n"
						   "PackProvider=modrinth\n"
						   "PackIconUrl=https://cdn.example.org/icon.png\n",
						   QDateTime::currentDateTime()));

		InstanceList list(globalSettings, {instsDir.path()});
		QCOMPARE(list.loadList(), InstanceList::NoError);

		QCOMPARE(list.roleNames().value(InstanceList::PackArtRole),
				 QByteArray("packArt"));
		const QModelIndex plain = list.getInstanceIndexById("plain");
		const QModelIndex pack = list.getInstanceIndexById("pack");
		QVERIFY(plain.isValid());
		QVERIFY(pack.isValid());
		QCOMPARE(list.data(plain, InstanceList::PackArtRole).toString(),
				 QString());
		QCOMPARE(list.data(pack, InstanceList::PackArtRole).toString(),
				 QStringLiteral("https://cdn.example.org/icon.png"));
	}

	/* PackCoverRole: empty for an instance not installed from a platform;
	 * for a platform pack whose recorded cover is already in the cache
	 * folder, that file - straight away, so a card never flashes its
	 * fallback first. No LauncherContext here, so nothing is fetched. */
	void packCoverRoleAnswersACachedCoverAtOnce()
	{
		QTemporaryDir globalDir;
		QVERIFY(globalDir.isValid());
		QTemporaryDir instsDir;
		QVERIFY(instsDir.isValid());
		QTemporaryDir coverDir;
		QVERIFY(coverDir.isValid());
		SettingsObjectPtr globalSettings = makeGlobalSettings(globalDir);

		const QString coverUrl =
			QStringLiteral("https://cdn.example.org/gallery/cover.png");
		const QString plainRoot = QDir(instsDir.path()).filePath("plain");
		const QString packRoot = QDir(instsDir.path()).filePath("pack");
		QVERIFY(QDir().mkpath(plainRoot));
		QVERIFY(QDir().mkpath(packRoot));
		QVERIFY(writeFile(QDir(plainRoot).filePath("instance.cfg"),
						   "InstanceType=NullTest\n"
						   "PackCoverUrl=https://cdn.example.org/gallery/cover.png\n",
						   QDateTime::currentDateTime()));
		QVERIFY(writeFile(QDir(packRoot).filePath("instance.cfg"),
						   "InstanceType=NullTest\n"
						   "PackProvider=modrinth\n"
						   "PackId=abcdEFGH\n"
						   "PackCoverUrl=https://cdn.example.org/gallery/cover.png\n",
						   QDateTime::currentDateTime()));
		const QString cached =
			PackArtwork::cachePath(coverDir.path(), coverUrl);
		QVERIFY(writeFile(cached, "png", QDateTime::currentDateTime()));

		InstanceList list(globalSettings, {instsDir.path()});
		list.setPackCoverDir(coverDir.path());
		QCOMPARE(list.loadList(), InstanceList::NoError);

		QCOMPARE(list.roleNames().value(InstanceList::PackCoverRole),
				 QByteArray("packCover"));
		const QModelIndex plain = list.getInstanceIndexById("plain");
		const QModelIndex pack = list.getInstanceIndexById("pack");
		QVERIFY(plain.isValid());
		QVERIFY(pack.isValid());
		// Not a platform pack: whatever its config says, no pack cover.
		QCOMPARE(list.data(plain, InstanceList::PackCoverRole).toString(),
				 QString());
		const QString expected = QUrl::fromLocalFile(cached).toString();
		QCOMPARE(list.data(pack, InstanceList::PackCoverRole).toString(),
				 expected);
		// And from memory afterwards, even once the file is gone.
		QVERIFY(QFile::remove(cached));
		QCOMPARE(list.data(pack, InstanceList::PackCoverRole).toString(),
				 expected);
	}

	/* A recorded cover that is not downloaded yet, with no launcher to
	 * download it: empty, without blocking or failing, and still empty
	 * once the deferred work has had its turn. */
	void packCoverRoleEmptyUntilDownloaded()
	{
		QTemporaryDir globalDir;
		QVERIFY(globalDir.isValid());
		QTemporaryDir instsDir;
		QVERIFY(instsDir.isValid());
		QTemporaryDir coverDir;
		QVERIFY(coverDir.isValid());
		SettingsObjectPtr globalSettings = makeGlobalSettings(globalDir);

		const QString packRoot = QDir(instsDir.path()).filePath("pack");
		QVERIFY(QDir().mkpath(packRoot));
		QVERIFY(writeFile(QDir(packRoot).filePath("instance.cfg"),
						   "InstanceType=NullTest\n"
						   "PackProvider=curseforge\n"
						   "PackId=925200\n"
						   "PackCoverUrl=https://media.example.org/shot.jpg\n",
						   QDateTime::currentDateTime()));

		InstanceList list(globalSettings, {instsDir.path()});
		list.setPackCoverDir(coverDir.path());
		QCOMPARE(list.loadList(), InstanceList::NoError);
		const QModelIndex pack = list.getInstanceIndexById("pack");
		QVERIFY(pack.isValid());

		QSignalSpy dataChangedSpy(&list, &InstanceList::dataChanged);
		QCOMPARE(list.data(pack, InstanceList::PackCoverRole).toString(),
				 QString());
		QTest::qWait(50);
		QCOMPARE(list.data(pack, InstanceList::PackCoverRole).toString(),
				 QString());
		QVERIFY(dataChangedSpy.isEmpty());
	}

	/* PackCoverUrl tells "never looked" (absent) from "looked, none"
	 * (empty), and the second survives the instance being read again -
	 * which is what keeps a pack with no cover from being looked up on
	 * every launch. */
	void packCoverRecordDistinguishesNoneFromUnknown()
	{
		QTemporaryDir globalDir;
		QVERIFY(globalDir.isValid());
		QTemporaryDir instsDir;
		QVERIFY(instsDir.isValid());
		SettingsObjectPtr globalSettings = makeGlobalSettings(globalDir);

		const QString packRoot = QDir(instsDir.path()).filePath("pack");
		QVERIFY(QDir().mkpath(packRoot));
		QVERIFY(writeFile(QDir(packRoot).filePath("instance.cfg"),
						   "InstanceType=NullTest\n"
						   "PackProvider=modrinth\n"
						   "PackId=abcdEFGH\n",
						   QDateTime::currentDateTime()));
		{
			InstanceList list(globalSettings, {instsDir.path()});
			QCOMPARE(list.loadList(), InstanceList::NoError);
			InstancePtr inst = list.getInstanceById("pack");
			QVERIFY(inst);
			QVERIFY(!inst->hasManagedPackCoverRecord());
			QCOMPARE(inst->managedPackCoverUrl(), QString());
			inst->setManagedPackCoverUrl(QString());
			QVERIFY(inst->hasManagedPackCoverRecord());
			inst->saveNow();
		}
		InstanceList reread(globalSettings, {instsDir.path()});
		QCOMPARE(reread.loadList(), InstanceList::NoError);
		InstancePtr inst = reread.getInstanceById("pack");
		QVERIFY(inst);
		QVERIFY(inst->hasManagedPackCoverRecord());
		QCOMPARE(inst->managedPackCoverUrl(), QString());
	}

	/* What a catalogue's record of a pack gives, per platform. */
	void packArtworkParsesEachCatalogue()
	{
		// Modrinth: the featured image, at full size, over the first one.
		const PackArtwork::Found modrinth = PackArtwork::parseLookup(
			QStringLiteral("modrinth"),
			reply("{'id': 'abcdEFGH', 'title': 'Some Pack',"
				"'icon_url': 'https://cdn.modrinth.com/data/abcdEFGH/icon.webp',"
				"'gallery': ["
				"{'url': 'https://cdn.modrinth.com/a_350.webp',"
				"'raw_url': 'https://cdn.modrinth.com/a.png', 'featured': false},"
				"{'url': 'https://cdn.modrinth.com/b_350.webp',"
				"'raw_url': 'https://cdn.modrinth.com/b.png', 'featured': true}]}"));
		QVERIFY(modrinth.recognised);
		QCOMPARE(modrinth.title, QStringLiteral("Some Pack"));
		QCOMPARE(modrinth.cover, QStringLiteral("https://cdn.modrinth.com/b.png"));
		QCOMPARE(modrinth.logo,
				 QStringLiteral("https://cdn.modrinth.com/data/abcdEFGH/icon.webp"));

		// None featured: the first; no raw_url: the url.
		const PackArtwork::Found unfeatured = PackArtwork::parseLookup(
			QStringLiteral("modrinth"),
			reply("{'id': 'x', 'gallery': [{'url': 'https://cdn.modrinth.com/first.jpg'},"
				"{'url': 'https://cdn.modrinth.com/second.jpg'}]}"));
		QCOMPARE(unfeatured.cover,
				 QStringLiteral("https://cdn.modrinth.com/first.jpg"));

		// A project with no gallery is still a record: "none".
		const PackArtwork::Found bare = PackArtwork::parseLookup(
			QStringLiteral("modrinth"), reply("{'id': 'x', 'gallery': []}"));
		QVERIFY(bare.recognised);
		QCOMPARE(bare.cover, QString());

		// An error body is not: nothing may be recorded from it.
		QVERIFY(!PackArtwork::parseLookup(QStringLiteral("modrinth"),
										  reply("{'error': 'not_found'}"))
					 .recognised);
		QVERIFY(!PackArtwork::parseLookup(QStringLiteral("modrinth"),
										  "<html>gateway</html>")
					 .recognised);

		// CurseForge (also spelled "flame"): the first screenshot.
		const PackArtwork::Found flame = PackArtwork::parseLookup(
			QStringLiteral("Flame"),
			reply("{'data': {'id': 925200, 'name': 'All the Mods 10',"
				"'logo': {'url': 'https://media.forgecdn.net/logo.png'},"
				"'screenshots': [{'url': 'https://media.forgecdn.net/one.png'},"
				"{'url': 'https://media.forgecdn.net/two.png'}]}}"));
		QVERIFY(flame.recognised);
		QCOMPARE(flame.title, QStringLiteral("All the Mods 10"));
		QCOMPARE(flame.cover, QStringLiteral("https://media.forgecdn.net/one.png"));
		QCOMPARE(flame.logo, QStringLiteral("https://media.forgecdn.net/logo.png"));

		// modpacks.ch: the splash art, and the square art as the logo.
		const PackArtwork::Found ftb = PackArtwork::parseLookup(
			QStringLiteral("modpacksch"),
			reply("{'name': 'FTB Pack', 'art': ["
				"{'type': 'square', 'url': 'https://cdn.example/square.png'},"
				"{'type': 'splash', 'url': 'https://cdn.example/splash.png'}]}"));
		QVERIFY(ftb.recognised);
		QCOMPARE(ftb.cover, QStringLiteral("https://cdn.example/splash.png"));
		QCOMPARE(ftb.logo, QStringLiteral("https://cdn.example/square.png"));
		QVERIFY(!PackArtwork::parseLookup(QStringLiteral("modpacksch"),
										  reply("{'status': 'error'}"))
					 .recognised);

		// Platforms with no such record.
		QVERIFY(!PackArtwork::parseLookup(QStringLiteral("atlauncher"), "{}")
					 .recognised);
	}

	void packArtworkLooksUpOnlyWhereThereIsARecord()
	{
		const QUrl modrinth =
			PackArtwork::lookupUrl(QStringLiteral("modrinth"), QStringLiteral("abcdEFGH"));
		QVERIFY(modrinth.isValid());
		QVERIFY(modrinth.toString().endsWith(QLatin1String("/project/abcdEFGH")));
		QVERIFY(PackArtwork::lookupUrl(QStringLiteral("modpacksch"), QStringLiteral("23"))
					.toString()
					.endsWith(QLatin1String("public/modpack/23")));
		QVERIFY(PackArtwork::lookupUrl(QStringLiteral("modrinth"), QString()).isEmpty());
		QVERIFY(PackArtwork::lookupUrl(QStringLiteral("atlauncher"),
									   QStringLiteral("SkyFactory4"))
					.isEmpty());
		QVERIFY(PackArtwork::lookupUrl(QStringLiteral("technic"),
									   QStringLiteral("tekkit"))
					.isEmpty());
		// ATLauncher's logo is named by the pack's safe name alone.
		QVERIFY(PackArtwork::derivedLogoUrl(QStringLiteral("atlauncher"),
											QStringLiteral("SkyFactory4"))
					.endsWith(QLatin1String("launcher/images/skyfactory4.png")));
		QCOMPARE(PackArtwork::derivedLogoUrl(QStringLiteral("modrinth"),
											 QStringLiteral("abcdEFGH")),
				 QString());
	}

	/* A Discover gallery as the modpack models hand it to QML. */
	void packArtworkPicksTheGalleryCover()
	{
		const QVariantMap first{{"url", "https://cdn/a_350.webp"},
								{"rawUrl", "https://cdn/a.png"},
								{"featured", false}};
		const QVariantMap featured{{"url", "https://cdn/b_350.webp"},
								   {"rawUrl", "https://cdn/b.png"},
								   {"featured", true}};
		const QVariantMap plain{{"url", "https://cdn/c.jpg"}, {"featured", false}};
		QCOMPARE(PackArtwork::coverFromGallery({first, featured}),
				 QStringLiteral("https://cdn/b.png"));
		QCOMPARE(PackArtwork::coverFromGallery({first}),
				 QStringLiteral("https://cdn/a.png"));
		QCOMPARE(PackArtwork::coverFromGallery({plain}),
				 QStringLiteral("https://cdn/c.jpg"));
		QCOMPARE(PackArtwork::coverFromGallery({}), QString());
	}

	void packArtworkCachePathFollowsTheUrl()
	{
		const QString a = PackArtwork::cachePath(QStringLiteral("/c"),
												 QStringLiteral("https://cdn/a.png"));
		const QString b = PackArtwork::cachePath(QStringLiteral("/c"),
												 QStringLiteral("https://cdn/b.png"));
		QVERIFY(a != b);
		QVERIFY(a.startsWith(QLatin1String("/c/")));
		QVERIFY(a.endsWith(QLatin1String(".png")));
		QCOMPARE(PackArtwork::cachePath(QStringLiteral("/c"),
										QStringLiteral("https://cdn/a.png")),
				 a);
		// No usable extension: the reader goes by the content instead.
		QVERIFY(!PackArtwork::cachePath(QStringLiteral("/c"),
										QStringLiteral("https://cdn/image?id=3"))
					 .contains(QLatin1Char('.')));
	}

	/* Only the icons an install falls back to may give way to the pack's
	 * logo - never one the user picked. */
	void packArtworkReplacesOnlyStockIcons()
	{
		QVERIFY(PackArtwork::isStockIconKey("modrinth", "default"));
		QVERIFY(PackArtwork::isStockIconKey("modrinth", ""));
		QVERIFY(PackArtwork::isStockIconKey("modrinth", "modrinth"));
		QVERIFY(PackArtwork::isStockIconKey("curseforge", "flame"));
		QVERIFY(PackArtwork::isStockIconKey("flame", "flame"));
		QVERIFY(PackArtwork::isStockIconKey("curseforge", "ftb_logo"));
		QVERIFY(PackArtwork::isStockIconKey("modpacksch", "ftb_logo"));
		QVERIFY(PackArtwork::isStockIconKey("atlauncher", "grass"));

		QVERIFY(!PackArtwork::isStockIconKey("modrinth", "gold"));
		QVERIFY(!PackArtwork::isStockIconKey("modrinth", "flame"));
		QVERIFY(!PackArtwork::isStockIconKey("modrinth", "grass"));
		QVERIFY(!PackArtwork::isStockIconKey("curseforge", "steve"));
		// A pack logo installed already is the pack's own.
		QVERIFY(!PackArtwork::isStockIconKey("modrinth", "modrinth_abcdEFGH"));
		QVERIFY(!PackArtwork::isStockIconKey("curseforge", "curseforge_925200"));

		QCOMPARE(PackArtwork::iconKeyFor("Flame", "925200"),
				 QStringLiteral("curseforge_925200"));
		// The key is the file name up to its first dot.
		QCOMPARE(PackArtwork::iconKeyFor("atlauncher", "Sky.Factory 4"),
				 QStringLiteral("atlauncher_Sky_Factory_4"));
	}

	void coverImageRoleScansAsynchronouslyAndCachesResult()
	{
		QTemporaryDir globalDir;
		QVERIFY(globalDir.isValid());
		QTemporaryDir instsDir;
		QVERIFY(instsDir.isValid());
		SettingsObjectPtr globalSettings = makeGlobalSettings(globalDir);

		/* Canonicalized before anything is built from it: InstanceList
		 * itself canonicalizes every configured instance root (resolving
		 * a symlink like macOS's /var -> /private/var), so building
		 * expectedUrl from the raw, un-resolved QTemporaryDir path would
		 * compare two spellings of the same file and never match. */
		const QString instsRoot =
			QFileInfo(instsDir.path()).canonicalFilePath();
		const QString instRoot = QDir(instsRoot).filePath("testinst");
		QVERIFY(QDir().mkpath(instRoot));
		QVERIFY(writeFile(QDir(instRoot).filePath("instance.cfg"),
						   "InstanceType=NullTest\n",
						   QDateTime::currentDateTime()));
		const QString screenshotsDir =
			QDir(instRoot).filePath("screenshots");
		QVERIFY(QDir().mkpath(screenshotsDir));
		const QString shotPath = QDir(screenshotsDir).filePath("shot.png");
		QVERIFY(writeFile(shotPath, "x", QDateTime::currentDateTime()));
		const QString expectedUrl =
			QUrl::fromLocalFile(QFileInfo(shotPath).absoluteFilePath())
				.toString();

		InstanceList list(globalSettings, {instsDir.path()});
		QCOMPARE(list.loadList(), InstanceList::NoError);
		const QModelIndex idx = list.getInstanceIndexById("testinst");
		QVERIFY(idx.isValid());

		// Nothing cached yet - data() must answer immediately (empty)
		// rather than block on the directory scan, and repeated asks before
		// the scan comes back must not queue a second one (no direct probe
		// for that here, but a duplicate scan finishing later would still
		// only re-publish the same, correct URL below).
		QCOMPARE(list.data(idx, InstanceList::CoverImageRole).toString(),
				 QString());
		QCOMPARE(list.data(idx, InstanceList::CoverImageRole).toString(),
				 QString());

		QTRY_COMPARE(list.data(idx, InstanceList::CoverImageRole).toString(),
					 expectedUrl);
	}

	void coverImageRoleRescansAfterInstanceStops()
	{
		QTemporaryDir globalDir;
		QVERIFY(globalDir.isValid());
		QTemporaryDir instsDir;
		QVERIFY(instsDir.isValid());
		SettingsObjectPtr globalSettings = makeGlobalSettings(globalDir);

		// See coverImageRoleScansAsynchronouslyAndCachesResult() for why
		// this is canonicalized first.
		const QString instsRoot =
			QFileInfo(instsDir.path()).canonicalFilePath();
		const QString instRoot = QDir(instsRoot).filePath("testinst");
		QVERIFY(QDir().mkpath(instRoot));
		QVERIFY(writeFile(QDir(instRoot).filePath("instance.cfg"),
						   "InstanceType=NullTest\n",
						   QDateTime::currentDateTime()));
		const QString screenshotsDir =
			QDir(instRoot).filePath("screenshots");
		QVERIFY(QDir().mkpath(screenshotsDir));
		const QString shotPath = QDir(screenshotsDir).filePath("shot.png");
		QVERIFY(writeFile(shotPath, "x", QDateTime::currentDateTime()));
		const QString expectedUrl =
			QUrl::fromLocalFile(QFileInfo(shotPath).absoluteFilePath())
				.toString();

		InstanceList list(globalSettings, {instsDir.path()});
		QCOMPARE(list.loadList(), InstanceList::NoError);
		InstancePtr inst = list.getInstanceById("testinst");
		QVERIFY(inst);
		const QModelIndex idx = list.getInstanceIndexById("testinst");
		QVERIFY(idx.isValid());

		// Let the first scan land and populate the cache.
		QCOMPARE(list.data(idx, InstanceList::CoverImageRole).toString(),
				 QString());
		QTRY_COMPARE(list.data(idx, InstanceList::CoverImageRole).toString(),
					 expectedUrl);

		// A play session ending is exactly when a new screenshot tends to
		// appear, so the cached cover is dropped - immediately, not only
		// once a fresh scan happens to finish.
		inst->setRunning(true);
		inst->setRunning(false);
		QCOMPARE(list.data(idx, InstanceList::CoverImageRole).toString(),
				 QString());

		// data() above already asked again, which schedules a fresh scan;
		// it eventually lands the same (still correct) URL.
		QTRY_COMPARE(list.data(idx, InstanceList::CoverImageRole).toString(),
					 expectedUrl);
	}

	/* An install's watcher learns the id its commit names - the id is only
	 * settled then - and keeps it once the task is over, whatever is
	 * committed after. */
	void reportCommittedInstanceNamesTheInstallsOwnInstance()
	{
		QTemporaryDir globalDir;
		QVERIFY(globalDir.isValid());
		QTemporaryDir instsDir;
		QVERIFY(instsDir.isValid());
		InstanceList list(makeGlobalSettings(globalDir), {instsDir.path()});

		auto* task = new ScriptedTask();
		TaskWatcher watcher{Task::Ptr(task)};
		list.reportCommittedInstance(&watcher);
		task->start();
		QCOMPARE(watcher.instanceId(), QString());

		emit list.instanceSelectRequest(QStringLiteral("All the Mods 10"));
		task->driveSuccess();
		QCOMPARE(watcher.instanceId(), QStringLiteral("All the Mods 10"));

		emit list.instanceSelectRequest(QStringLiteral("something later"));
		QCOMPARE(watcher.instanceId(), QStringLiteral("All the Mods 10"));

		list.reportCommittedInstance(nullptr);
	}
};

QTEST_GUILESS_MAIN(InstanceListTest)

#include "InstanceList_test.moc"
