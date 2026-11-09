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
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QDir>
#include <QPalette>
#include <QStandardPaths>
#include <QXmlStreamReader>

#include <KIconEngine>
#include <KIconLoader>

#include "ui/themes/ThemedIcons.h"

#ifndef MESHMC_MAINWINDOW_UI_PATH
#error "MESHMC_MAINWINDOW_UI_PATH is not set; see add_unit_test(ThemedIcons) in launcher/CMakeLists.txt"
#endif

// The resources are in MeshMC_logic; nothing initialises them in a test.
static void initResources()
{
	Q_INIT_RESOURCE(multimc);
	Q_INIT_RESOURCE(pe_colored);
	Q_INIT_RESOURCE(pe_blue);
	Q_INIT_RESOURCE(pe_dark);
	Q_INIT_RESOURCE(pe_light);
}

namespace
{
/// One "meshmcIcon" property found in a .ui file: the icon name, and the
/// object that asked for it, so a failure names something greppable.
struct UiIconRequest {
	QString objectName;
	QString iconName;
};

/* Read every "meshmcIcon" dynamic property out of a Designer .ui file.
 *
 * MainWindow::applyThemedIcons() walks its own children looking for exactly
 * this property and assigns whatever the name resolves to, so this is the
 * complete list of icon names MainWindow asks for by way of the .ui -- and
 * the list the compiler cannot check, since the names are XML text.
 *
 * Parsed with QXmlStreamReader rather than matched with a regular
 * expression, so that the structure the reader is relying on (the value is a
 * <string> child of the property element) is the structure that is actually
 * required, and a .ui that stops having it fails loudly instead of quietly
 * yielding nothing.
 */
QList<UiIconRequest> readUiIconRequests(const QString& uiPath, QString* error)
{
	QList<UiIconRequest> found;

	QFile file(uiPath);
	if (!file.open(QIODevice::ReadOnly)) {
		*error = uiPath + QStringLiteral(": ") + file.errorString();
		return found;
	}

	QXmlStreamReader xml(&file);

	/* The nearest enclosing named element. Designer nests a property inside
	 * the <action> or <widget> it belongs to, and nothing else carries a
	 * name we would want in a failure message. */
	QString owner;

	while (!xml.atEnd()) {
		if (xml.readNext() != QXmlStreamReader::StartElement) {
			continue;
		}

		const QString element = xml.name().toString();
		if (element == QStringLiteral("action") ||
			element == QStringLiteral("widget")) {
			owner = xml.attributes()
						.value(QStringLiteral("name"))
						.toString();
			continue;
		}
		if (element != QStringLiteral("property")) {
			continue;
		}
		if (xml.attributes().value(QStringLiteral("name")).toString() !=
			QStringLiteral("meshmcIcon")) {
			continue;
		}

		QString value;
		while (!xml.atEnd()) {
			const QXmlStreamReader::TokenType token = xml.readNext();
			if (token == QXmlStreamReader::StartElement &&
				xml.name().toString() == QStringLiteral("string")) {
				value = xml.readElementText();
				break;
			}
			if (token == QXmlStreamReader::EndElement &&
				xml.name().toString() == QStringLiteral("property")) {
				break;
			}
		}

		if (value.isEmpty()) {
			*error = QStringLiteral("%1: the meshmcIcon property on '%2' has "
									"no <string> value")
						 .arg(uiPath, owner);
			return found;
		}
		found.append({owner, value});
	}

	if (xml.hasError()) {
		*error = QStringLiteral("%1:%2: %3")
					 .arg(uiPath)
					 .arg(xml.lineNumber())
					 .arg(xml.errorString());
	}
	return found;
}

	/// The average colour of the opaque pixels of \p icon.
	QColor glyphColor(const QIcon& icon, int size = 32)
	{
		const QImage image =
			icon.pixmap(size).toImage().convertToFormat(QImage::Format_ARGB32);
		qint64 r = 0, g = 0, b = 0, n = 0;
		for (int y = 0; y < image.height(); ++y) {
			for (int x = 0; x < image.width(); ++x) {
				const QColor c = image.pixelColor(x, y);
				if (c.alpha() > 200) {
					r += c.red();
					g += c.green();
					b += c.blue();
					++n;
				}
			}
		}
		return n ? QColor(r / n, g / n, b / n) : QColor();
	}
} // namespace

class ThemedIconsTest : public QObject
{
	Q_OBJECT

  private slots:
	void initTestCase()
	{
		// the user's icon themes are in a folder of the test, not of the user
		QStandardPaths::setTestModeEnabled(true);
		QDir(ThemedIcons::userDirectory()).removeRecursively();
		initResources();
		ThemedIcons::init();
	}

	void cleanupTestCase()
	{
		ThemedIcons::setTheme(QStringLiteral("breeze"));
		QDir(ThemedIcons::userDirectory()).removeRecursively();
	}

	void test_BreezeIsTheDefaultTheme()
	{
		QCOMPARE(ThemedIcons::theme(), QString("breeze"));
		const auto themes = ThemedIcons::themes();
		QVERIFY(!themes.isEmpty());
		QCOMPARE(themes.first().id, QString("breeze"));
	}

	void test_ThemeOfTheUserIsListedAndUsed()
	{
		const QString dir = ThemedIcons::userDirectory() + "/mytheme";
		QVERIFY(QDir().mkpath(dir + "/scalable/actions"));
		QFile index(dir + "/index.theme");
		QVERIFY(index.open(QIODevice::WriteOnly));
		index.write("[Icon Theme]\nName=My Theme\nInherits=breeze\n"
					"Directories=scalable/actions\n\n[scalable/actions]\n"
					"Size=48\nType=Scalable\nMinSize=8\nMaxSize=512\n"
					"Context=Actions\n");
		index.close();

		// found without the application being restarted
		ThemedIcons::reload();
		bool listed = false;
		for (const auto& theme : ThemedIcons::themes()) {
			if (theme.id == "mytheme") {
				listed = true;
				QCOMPARE(theme.name, QString("My Theme"));
			}
		}
		QVERIFY(listed);

		QCOMPARE(ThemedIcons::setTheme("mytheme"), QString("mytheme"));
		QCOMPARE(ThemedIcons::theme(), QString("mytheme"));
		// it inherits Breeze, so what it lacks is still found
		QVERIFY(!ThemedIcons::icon("copy").isNull());
		QVERIFY(!ThemedIcons::icon("minecraft").isNull());

		QCOMPARE(ThemedIcons::setTheme("breeze"), QString("breeze"));
	}

	void test_AnAliasIsAnIconWhileTheLoaderIsMadeAgain()
	{
		// the icons that only Breeze has (the bundled theme has none of
		// them) must not come out empty, whatever the icon loader was doing
		// when they were asked for
		ThemedIcons::setTheme("breeze");
		KIconLoader::global()->newIconLoader();
		for (const QString& name : {"accounts", "externaltools", "notes"}) {
			const QIcon icon = ThemedIcons::icon(name);
			QVERIFY2(!icon.isNull(), qPrintable(name));
			QVERIFY2(!icon.pixmap(32).isNull(), qPrintable(name));
			QVERIFY2(glyphColor(icon).isValid(), qPrintable(name));
		}
	}

	void test_EveryThemeShowsTheIconsOfTheApplication()
	{
		// the bundled theme has none of Breeze's icons; choosing it (or going
		// back to Breeze afterwards) must not leave icons empty
		const QStringList names = {"accounts", "copy",	   "notes",
								   "settings", "launch",   "externaltools"};
		for (const char* theme : {"multimc", "breeze", "multimc", "breeze"}) {
			QCOMPARE(ThemedIcons::setTheme(theme), QString(theme));
			for (const QString& name : names) {
				QVERIFY2(glyphColor(ThemedIcons::icon(name)).isValid(),
						 qPrintable(QString(theme) + ": " + name));
			}
			// the icons of instances are only in the bundled theme
			for (const char* name : {"grass", "creeper"}) {
				QVERIFY2(!QIcon::fromTheme(QString::fromLatin1(name)).isNull(),
						 qPrintable(QString(theme) + ": " + name));
			}
		}
	}

	void test_InheritedIconsRemainVisibleAcrossThemeAndPaletteChanges()
	{
		ThemedIcons::setTheme("breeze");
		const QStringList names = {"copy", "launch", "new", "viewfolder",
								   "accounts", "notes"};
		QList<QIcon> retained;
		for (const QString& name : names) {
			retained.append(ThemedIcons::icon(name));
		}

		for (bool dark : {true, false, true}) {
			QPalette palette = QGuiApplication::palette();
			const QColor foreground = dark ? QColor(239, 240, 241)
										   : QColor(35, 38, 41);
			const QColor background = dark ? QColor(35, 38, 41) : Qt::white;
			palette.setColor(QPalette::Window, background);
			for (auto role : {QPalette::WindowText, QPalette::Text,
							  QPalette::ButtonText}) {
				palette.setColor(role, foreground);
			}
			ThemedIcons::setPalette(palette);

			for (const char* theme : {"breeze", "multimc", "breeze"}) {
				QCOMPARE(ThemedIcons::setTheme(theme), QString(theme));
				for (int i = 0; i < names.size(); ++i) {
					// Both existing widgets and newly opened pages must remain
					// readable. A non-null, opaque pixmap alone is not enough.
					for (const QIcon& icon :
						 {retained[i], ThemedIcons::icon(names[i])}) {
						// Breeze uses monochrome artwork at toolbar size;
						// larger sizes can have intentionally coloured artwork.
						const QColor actual = glyphColor(icon, 22);
						const QString context = QString("%1: %2, dark=%3, color=%4")
							.arg(theme, names[i]).arg(dark).arg(actual.name());
						QVERIFY2(actual.isValid(), qPrintable(context));
						QVERIFY2(qAbs(actual.lightness() - background.lightness()) > 100,
								 qPrintable(context));
					}
				}
			}
		}
		ThemedIcons::setPalette(QGuiApplication::palette());
	}

	void test_BundledPeThemesUseTheirOwnArtwork_data()
	{
		QTest::addColumn<QString>("theme");
		for (const char* theme : {"pe_colored", "pe_blue", "pe_dark", "pe_light"}) {
			QTest::newRow(theme) << QString::fromLatin1(theme);
		}
	}

	void test_BundledPeThemesUseTheirOwnArtwork()
	{
		QFETCH(QString, theme);
		QCOMPARE(ThemedIcons::setTheme("breeze"), QString("breeze"));
		QHash<QString, QIcon> retained;
		QHash<QString, QImage> breeze;
		for (auto it = ThemedIcons::aliases().cbegin();
			 it != ThemedIcons::aliases().cend(); ++it) {
			retained.insert(it.key(), ThemedIcons::icon(it.key()));
			breeze.insert(it.key(), retained[it.key()].pixmap(32).toImage());
		}

		for (int cycle = 0; cycle < 2; ++cycle) {
			QCOMPARE(ThemedIcons::setTheme(theme), theme);
			for (auto it = retained.cbegin(); it != retained.cend(); ++it) {
				const QString path = QString(":/icons/%1/scalable/%2.svg")
					.arg(theme, it.key());
				QVERIFY2(QFile::exists(path), qPrintable(path));
				// Render the original artwork directly, bypassing theme-name
				// lookup. A nonempty fallback is not the selected theme's icon.
				const QIcon expected(new KIconEngine(path, KIconLoader::global()));
				QVERIFY2(glyphColor(expected, 22).isValid(), qPrintable(path));
				for (int size : {22, 32}) {
					for (qreal scale : {1.0, 2.0}) {
						const QImage image = expected.pixmap(QSize(size, size), scale).toImage();
						QVERIFY2(!image.isNull(), qPrintable(path));
						for (const QIcon& icon : {it.value(), ThemedIcons::icon(it.key())}) {
							QVERIFY2(!icon.isNull(), qPrintable(theme + ": " + it.key()));
							QVERIFY2(icon.pixmap(QSize(size, size), scale).toImage() == image,
									 qPrintable(theme + ": " + it.key()));
						}
					}
				}
			}
			// Both retained icons and newly requested icons must switch back.
			QCOMPARE(ThemedIcons::setTheme("breeze"), QString("breeze"));
			for (auto it = retained.cbegin(); it != retained.cend(); ++it) {
				QCOMPARE(it.value().pixmap(32).toImage(), breeze.value(it.key()));
				QCOMPARE(ThemedIcons::icon(it.key()).pixmap(32).toImage(), breeze.value(it.key()));
			}
		}
	}

	void test_ThemesDoesNotResetTheIconLoader()
	{
		// filling a list must not change what the loader has been told
		QPalette palette;
		palette.setColor(QPalette::WindowText, QColor(10, 200, 30));
		ThemedIcons::setPalette(palette);
		const QColor before = glyphColor(ThemedIcons::icon("copy"));
		ThemedIcons::themes();
		ThemedIcons::themes();
		QCOMPARE(glyphColor(ThemedIcons::icon("copy")), before);
	}

	void test_AThemeThatIsNotThereIsBreeze()
	{
		QCOMPARE(ThemedIcons::setTheme("no-such-meshmc-theme"), QString("breeze"));
		QCOMPARE(ThemedIcons::theme(), QString("breeze"));
	}

	void test_EveryAliasIsAnIconOfBreeze()
	{
		for (auto it = ThemedIcons::aliases().cbegin();
			 it != ThemedIcons::aliases().cend(); ++it) {
			QVERIFY2(KIconLoader::global()->hasIcon(it.value()),
					 qPrintable(it.key() + " -> " + it.value()));
		}
	}

	void test_EveryIconTheCodeAsksForIsThere()
	{
		// the names the code and the pages pass to getThemedIcon(), and the
		// ones the main window's .ui has; "logo" is the application's own
		// icon, and is not looked up in a theme
		const QStringList names = {
			"about",		"accounts",	   "appearance",   "atlauncher",
			"atlauncher-placeholder", "backup", "bug",	   "centralmods",
			"checkupdate",	"copy",		   "custom-commands", "delete",
			"discord",		"export",	   "externaltools", "flame",
			"ftb_logo",		"help",		   "instance-settings", "java",
			"launch",		"launcher",	   "loadermods",   "log",
			"minecraft",	"modrinth",	   "new",		   "news",
			"noaccount",	"notes",	   "patreon",	   "plugins",
			"proxy",		"reddit-alien", "rename",	   "resourcepacks",
			"screenshot-placeholder", "screenshots", "settings", "shortcut",
			"star",			"status-bad",  "status-good",  "status-yellow",
			"tag",			"technic",	   "unknown_server", "viewfolder",
			"worlds"};
		for (const QString& name : names) {
			QVERIFY2(!ThemedIcons::icon(name).isNull(), qPrintable(name));
		}
	}

	void test_EveryIconTheMainWindowAsksForByItsUiIsThere()
	{
		QString error;
		const QList<UiIconRequest> requests =
			readUiIconRequests(QStringLiteral(MESHMC_MAINWINDOW_UI_PATH),
							   &error);
		QVERIFY2(error.isEmpty(), qPrintable(error));
		// a reader that found nothing would pass for the wrong reason
		QVERIFY2(requests.size() > 10, "too few meshmcIcon properties found");
		for (const UiIconRequest& request : requests) {
			QVERIFY2(!ThemedIcons::icon(request.iconName).isNull(),
					 qPrintable(request.objectName + ": " +
								request.iconName));
		}
	}

	void test_TheIconsOfInstancesAreFound()
	{
		// MMCIcon asks QIcon for these by name, and they are only in the
		// bundled theme, which Breeze does not have
		for (const char* name : {"grass", "creeper", "diamond"}) {
			QVERIFY2(!QIcon::fromTheme(QString::fromLatin1(name)).isNull(),
					 name);
		}
	}

	void test_AnIconThatIsNowhereIsNull()
	{
		QVERIFY(ThemedIcons::icon("no-such-icon-anywhere").isNull());
	}

	void test_AnAliasFollowsThePalette()
	{
		const QPalette original = QGuiApplication::palette();

		QPalette light = original;
		light.setColor(QPalette::WindowText, QColor(20, 20, 20));
		light.setColor(QPalette::ButtonText, QColor(20, 20, 20));
		light.setColor(QPalette::Text, QColor(20, 20, 20));
		ThemedIcons::setPalette(light);
		const QColor onLight = glyphColor(ThemedIcons::icon("copy"));

		QPalette dark = original;
		dark.setColor(QPalette::WindowText, QColor(240, 240, 240));
		dark.setColor(QPalette::ButtonText, QColor(240, 240, 240));
		dark.setColor(QPalette::Text, QColor(240, 240, 240));
		ThemedIcons::setPalette(dark);
		const QColor onDark = glyphColor(ThemedIcons::icon("copy"));

		QVERIFY(onLight.isValid() && onDark.isValid());
		QVERIFY2(onDark.lightness() > 200, qPrintable(onDark.name()));
		QVERIFY2(onLight.lightness() < 100, qPrintable(onLight.name()));
	}
};

QTEST_MAIN(ThemedIconsTest)

#include "ThemedIcons_test.moc"
