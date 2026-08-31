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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "ui/themes/CustomTheme.h"
#include "ui/themes/ITheme.h"

/*
 * Behavioural contract for CustomTheme.
 *
 * CustomTheme resolves paths relative to the process working directory
 * ("themes/<id>/..."), so every test runs inside a throwaway directory that is
 * made current for the duration of the test case.
 */

namespace
{

/// Minimal ITheme whose values are all distinctive, so that "inherited from the
/// base theme" is always distinguishable from "read from the theme files".
class StubBaseTheme : public ITheme
{
  public:
	static const char* styleSheet()
	{
		return "QWidget { color: #010203; }";
	}
	static QColor windowColor()
	{
		return QColor("#111111");
	}

	QString id() override
	{
		return "stub-base";
	}
	QString name() override
	{
		return "Stub Base";
	}
	bool hasStyleSheet() override
	{
		return true;
	}
	QString appStyleSheet() override
	{
		return styleSheet();
	}
	QString qtTheme() override
	{
		return "StubWidgets";
	}
	bool hasColorScheme() override
	{
		return true;
	}
	QPalette colorScheme() override
	{
		QPalette palette;
		palette.setColor(QPalette::Window, windowColor());
		palette.setColor(QPalette::WindowText, QColor("#eeeeee"));
		palette.setColor(QPalette::Base, QColor("#121212"));
		palette.setColor(QPalette::Text, QColor("#dddddd"));
		palette.setColor(QPalette::Highlight, QColor("#334455"));
		return palette;
	}
	QColor fadeColor() override
	{
		return QColor("#222222");
	}
	double fadeAmount() override
	{
		return 0.25;
	}
};

/// Writes \a data to \a path, creating parent directories as needed.
bool writeFile(const QString& path, const QByteArray& data)
{
	if (!QFileInfo(path).absoluteDir().mkpath(".")) {
		return false;
	}
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return false;
	}
	const bool written = file.write(data) == data.size();
	file.close();
	return written;
}

QByteArray readFile(const QString& path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return {};
	}
	return file.readAll();
}

} // namespace

class CustomThemeTest : public QObject
{
	Q_OBJECT

  private:
	QTemporaryDir m_tempDir;
	QString m_previousCwd;
	StubBaseTheme m_base;

	/// Absolute path of themes/<folder>, created on demand.
	QString themeDir(const QString& folder)
	{
		QString path = QDir::current().absoluteFilePath("themes/" + folder);
		QDir().mkpath(path);
		return path;
	}

  private slots:
	void initTestCase()
	{
		QVERIFY2(m_tempDir.isValid(), "could not create a temporary directory");
		m_previousCwd = QDir::currentPath();
		QVERIFY(QDir::setCurrent(m_tempDir.path()));
	}

	void cleanupTestCase()
	{
		if (!m_previousCwd.isEmpty()) {
			QDir::setCurrent(m_previousCwd);
		}
	}

	// ---------------------------------------------------------------------
	// Manifest themes
	// ---------------------------------------------------------------------

	/// The theme id comes from the folder, everything else from theme.json.
	void test_manifest_readsIdentityAndPalette()
	{
		QString dir = themeDir("manifest-basic");
		QVERIFY(writeFile(dir + "/theme.json", R"({
			"name": "Basic Manifest",
			"widgets": "Fusion",
			"colors": {
				"Window": "#0a0b0c",
				"WindowText": "#fafbfc",
				"Highlight": "#123456",
				"fadeColor": "#654321",
				"fadeAmount": 0.75
			}
		})"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		// id is the folder name, NOT the file name and NOT the json "name"
		QCOMPARE(theme->id(), QStringLiteral("manifest-basic"));
		QCOMPARE(theme->name(), QStringLiteral("Basic Manifest"));
		QCOMPARE(theme->qtTheme(), QStringLiteral("Fusion"));
		QCOMPARE(theme->fadeAmount(), 0.75);
		QCOMPARE(theme->fadeColor(), QColor("#654321"));
		QVERIFY(theme->hasStyleSheet());
		QVERIFY(theme->hasColorScheme());

		// Declared colours win over the base theme's.
		QPalette palette = theme->colorScheme();
		QCOMPARE(palette.color(QPalette::Active, QPalette::Window),
				 QColor("#0a0b0c"));
		QCOMPARE(palette.color(QPalette::Active, QPalette::WindowText),
				 QColor("#fafbfc"));
		QCOMPARE(palette.color(QPalette::Active, QPalette::Highlight),
				 QColor("#123456"));

		// Undeclared roles keep coming from the base theme.
		QCOMPARE(palette.color(QPalette::Active, QPalette::Base),
				 QColor("#121212"));
	}

	/// fadeInactive() must be applied, i.e. the Disabled group is derived from
	/// the Active one rather than left identical to it.
	void test_manifest_fadesDisabledColours()
	{
		QString dir = themeDir("manifest-fade");
		QVERIFY(writeFile(dir + "/theme.json", R"({
			"name": "Fade",
			"widgets": "Fusion",
			"colors": {
				"Window": "#00ff00",
				"fadeColor": "#000000",
				"fadeAmount": 0.5
			}
		})"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QPalette palette = theme->colorScheme();
		QCOMPARE(palette.color(QPalette::Active, QPalette::Window),
				 QColor("#00ff00"));
		QVERIFY2(palette.color(QPalette::Disabled, QPalette::Window) !=
					 palette.color(QPalette::Active, QPalette::Window),
				 "Disabled colours were not faded");
	}

	/// "colors" is optional; the palette is then inherited wholesale.
	void test_manifest_colorsBlockIsOptional()
	{
		QString dir = themeDir("manifest-nocolors");
		QVERIFY(writeFile(dir + "/theme.json", R"({
			"name": "No Colours",
			"widgets": "Fusion"
		})"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QCOMPARE(theme->name(), QStringLiteral("No Colours"));
		QCOMPARE(theme->colorScheme().color(QPalette::Active, QPalette::Window),
				 StubBaseTheme::windowColor());
	}

	/// Default stylesheet file name is themeStyle.css.
	void test_manifest_defaultStyleSheetName()
	{
		QString dir = themeDir("manifest-defaultcss");
		QVERIFY(writeFile(dir + "/theme.json", R"({
			"name": "Default CSS",
			"widgets": "Fusion"
		})"));
		QVERIFY(writeFile(dir + "/themeStyle.css", "QLabel { color: red; }"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QCOMPARE(theme->appStyleSheet(), QStringLiteral("QLabel { color: red; }"));
	}

	/// "qssFilePath" redirects the stylesheet to another file in the folder.
	void test_manifest_qssFilePathIsHonoured()
	{
		QString dir = themeDir("manifest-qsspath");
		QVERIFY(writeFile(dir + "/theme.json", R"({
			"name": "Custom CSS Name",
			"widgets": "Fusion",
			"qssFilePath": "elsewhere.qss"
		})"));
		QVERIFY(writeFile(dir + "/elsewhere.qss", "QLabel { color: blue; }"));
		// A file with the default name must be ignored when qssFilePath is set.
		QVERIFY(writeFile(dir + "/themeStyle.css", "QLabel { color: red; }"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QCOMPARE(theme->appStyleSheet(),
				 QStringLiteral("QLabel { color: blue; }"));
	}

	/// With no stylesheet on disk the base theme's stylesheet is kept.
	void test_manifest_missingStyleSheetFallsBackToBase()
	{
		QString dir = themeDir("manifest-nocss");
		QVERIFY(writeFile(dir + "/theme.json", R"({
			"name": "No CSS",
			"widgets": "Fusion"
		})"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QCOMPARE(theme->appStyleSheet(),
				 QString(StubBaseTheme::styleSheet()));
	}

	/// Optional metadata is composed into the tooltip, in a stable order.
	void test_manifest_tooltipFromMetadata()
	{
		QString dir = themeDir("manifest-tooltip");
		QVERIFY(writeFile(dir + "/theme.json", R"({
			"name": "Documented",
			"widgets": "Fusion",
			"description": "A described theme.",
			"author": "Some One",
			"license": "MIT"
		})"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QStringList lines = theme->tooltip().split('\n');
		QCOMPARE(lines.size(), 3);
		QCOMPARE(lines.at(0), QStringLiteral("A described theme."));
		QVERIFY2(lines.at(1).contains("Some One"),
				 qPrintable("author line was: " + lines.at(1)));
		QVERIFY2(lines.at(2).contains("MIT"),
				 qPrintable("license line was: " + lines.at(2)));
	}

	/// Metadata is genuinely optional: no fields means no tooltip.
	void test_manifest_tooltipEmptyWithoutMetadata()
	{
		QString dir = themeDir("manifest-notooltip");
		QVERIFY(writeFile(dir + "/theme.json", R"({
			"name": "Bare",
			"widgets": "Fusion"
		})"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QVERIFY(theme->tooltip().isEmpty());
	}

	/// Partial metadata must not produce blank tooltip lines.
	void test_manifest_tooltipSkipsMissingFields()
	{
		QString dir = themeDir("manifest-partialtooltip");
		QVERIFY(writeFile(dir + "/theme.json", R"({
			"name": "Half Documented",
			"widgets": "Fusion",
			"author": "Only Author"
		})"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QStringList lines = theme->tooltip().split('\n', Qt::SkipEmptyParts);
		QCOMPARE(lines.size(), 1);
		QVERIFY(lines.at(0).contains("Only Author"));
	}

	/// A theme.json that does not exist leaves the theme on base values, but
	/// still identifiable by its folder.
	void test_manifest_missingFileFallsBackToBase()
	{
		QString dir = themeDir("manifest-absent");

		QFileInfo manifest(dir + "/theme.json");
		QVERIFY(!manifest.exists());
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QCOMPARE(theme->id(), QStringLiteral("manifest-absent"));
		QCOMPARE(theme->name(), QStringLiteral("manifest-absent"));
		QCOMPARE(theme->qtTheme(), QStringLiteral("StubWidgets"));
		QCOMPARE(theme->appStyleSheet(), QString(StubBaseTheme::styleSheet()));
		QCOMPARE(theme->colorScheme().color(QPalette::Active, QPalette::Window),
				 StubBaseTheme::windowColor());
	}

	/// Unparseable JSON must not take the application down or half-apply.
	void test_manifest_malformedJsonFallsBackToBase()
	{
		QString dir = themeDir("manifest-malformed");
		QVERIFY(writeFile(dir + "/theme.json", "{ this is not json"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QCOMPARE(theme->id(), QStringLiteral("manifest-malformed"));
		QCOMPARE(theme->name(), QStringLiteral("manifest-malformed"));
		QCOMPARE(theme->qtTheme(), QStringLiteral("StubWidgets"));
		QCOMPARE(theme->colorScheme().color(QPalette::Active, QPalette::Window),
				 StubBaseTheme::windowColor());
	}

	/// "widgets" is mandatory: without it the theme is rejected, and the
	/// palette must not be left partially overridden.
	void test_manifest_missingRequiredWidgetsIsRejected()
	{
		QString dir = themeDir("manifest-nowidgets");
		QVERIFY(writeFile(dir + "/theme.json", R"({
			"name": "Incomplete",
			"colors": { "Window": "#abcdef" }
		})"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QCOMPARE(theme->qtTheme(), QStringLiteral("StubWidgets"));
		QCOMPARE(theme->colorScheme().color(QPalette::Active, QPalette::Window),
				 StubBaseTheme::windowColor());
	}

	/// Colour strings that Qt cannot parse are skipped, not fatal.
	void test_manifest_invalidColourIsSkipped()
	{
		QString dir = themeDir("manifest-badcolor");
		QVERIFY(writeFile(dir + "/theme.json", R"({
			"name": "Bad Colour",
			"widgets": "Fusion",
			"colors": {
				"Window": "not-a-colour",
				"WindowText": "#abcdef"
			}
		})"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QPalette palette = theme->colorScheme();
		// Bad value ignored, base kept.
		QCOMPARE(palette.color(QPalette::Active, QPalette::Window),
				 StubBaseTheme::windowColor());
		// Sibling value in the same block still applied.
		QCOMPARE(palette.color(QPalette::Active, QPalette::WindowText),
				 QColor("#abcdef"));
	}

	/// fadeAmount defaults to 0.5 when the colours block omits it.
	void test_manifest_fadeAmountDefault()
	{
		QString dir = themeDir("manifest-nofade");
		QVERIFY(writeFile(dir + "/theme.json", R"({
			"name": "No Fade Amount",
			"widgets": "Fusion",
			"colors": { "Window": "#0a0a0a" }
		})"));

		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QCOMPARE(theme->fadeAmount(), 0.5);
	}

	// ---------------------------------------------------------------------
	// Plain stylesheet themes
	// ---------------------------------------------------------------------

	/// A bare .qss file is a theme identified by its file name.
	void test_stylesheet_identityAndContents()
	{
		QString dir = themeDir("plain");
		QVERIFY(writeFile(dir + "/midnight.qss", "QMainWindow { color: #fff; }"));

		QFileInfo sheet(dir + "/midnight.qss");
		auto theme = CustomTheme::fromStyleSheet(&m_base, sheet);

		// id keeps the extension, name drops it
		QCOMPARE(theme->id(), QStringLiteral("midnight.qss"));
		QCOMPARE(theme->name(), QStringLiteral("midnight"));
		QCOMPARE(theme->appStyleSheet(),
				 QStringLiteral("QMainWindow { color: #fff; }"));
		QVERIFY(theme->hasStyleSheet());
	}

	/// Stylesheet themes inherit the whole palette and widget style.
	void test_stylesheet_inheritsBaseTheme()
	{
		QString dir = themeDir("plain-inherit");
		QVERIFY(writeFile(dir + "/x.css", "QLabel {}"));

		QFileInfo sheet(dir + "/x.css");
		auto theme = CustomTheme::fromStyleSheet(&m_base, sheet);

		QCOMPARE(theme->qtTheme(), QStringLiteral("StubWidgets"));
		QCOMPARE(theme->fadeAmount(), 0.25);
		QCOMPARE(theme->fadeColor(), QColor("#222222"));
		QCOMPARE(theme->colorScheme().color(QPalette::Active, QPalette::Window),
				 StubBaseTheme::windowColor());
		QVERIFY(theme->tooltip().isEmpty());
	}

	/// A stylesheet that cannot be read falls back to the base stylesheet.
	void test_stylesheet_unreadableFallsBackToBase()
	{
		QString dir = themeDir("plain-missing");

		QFileInfo sheet(dir + "/does-not-exist.qss");
		QVERIFY(!sheet.exists());
		auto theme = CustomTheme::fromStyleSheet(&m_base, sheet);

		QCOMPARE(theme->appStyleSheet(), QString(StubBaseTheme::styleSheet()));
	}

	// ---------------------------------------------------------------------
	// searchPaths()
	// ---------------------------------------------------------------------

	/// The resources folder is exposed only when it actually exists.
	void test_searchPaths_reportedOnlyWhenResourcesExist()
	{
		QString withRes = themeDir("res-yes");
		QVERIFY(writeFile(withRes + "/theme.json", R"({
			"name": "With Resources", "widgets": "Fusion"
		})"));
		QVERIFY(QDir().mkpath(withRes + "/resources"));

		QFileInfo manifest(withRes + "/theme.json");
		auto themed = CustomTheme::fromManifest(&m_base, manifest);
		QCOMPARE(themed->searchPaths().size(), 1);
		QVERIFY(themed->searchPaths().at(0).contains("res-yes"));
		QVERIFY(themed->searchPaths().at(0).contains("resources"));

		QString withoutRes = themeDir("res-no");
		QVERIFY(writeFile(withoutRes + "/theme.json", R"({
			"name": "No Resources", "widgets": "Fusion"
		})"));
		// Remove the folder CustomTheme's constructor may have created.
		QDir(withoutRes + "/resources").removeRecursively();

		QFileInfo manifest2(withoutRes + "/theme.json");
		auto bare = CustomTheme::fromManifest(&m_base, manifest2);
		QVERIFY(bare->searchPaths().isEmpty());
	}

	// ---------------------------------------------------------------------
	// writeSkeleton()
	// ---------------------------------------------------------------------

	/// A starter theme is generated with all expected files.
	void test_writeSkeleton_createsStarterTheme()
	{
		QVERIFY(CustomTheme::writeSkeleton(&m_base, "generated"));

		QString dir = QDir::current().absoluteFilePath("themes/generated");
		QVERIFY(QFileInfo::exists(dir + "/theme.json"));
		QVERIFY(QFileInfo::exists(dir + "/themeStyle.css"));
		QVERIFY(QFileInfo(dir + "/resources").isDir());

		// The stylesheet is seeded from the base theme.
		QCOMPARE(QString::fromUtf8(readFile(dir + "/themeStyle.css")),
				 QString(StubBaseTheme::styleSheet()));
	}

	/// What is written must be loadable again, with a presentable name.
	void test_writeSkeleton_outputIsLoadable()
	{
		QVERIFY(CustomTheme::writeSkeleton(&m_base, "roundtrip"));

		QString dir = QDir::current().absoluteFilePath("themes/roundtrip");
		QFileInfo manifest(dir + "/theme.json");
		auto theme = CustomTheme::fromManifest(&m_base, manifest);

		QCOMPARE(theme->id(), QStringLiteral("roundtrip"));
		// Folder name is capitalised for display.
		QCOMPARE(theme->name(), QStringLiteral("Roundtrip"));
		QCOMPARE(theme->qtTheme(), QStringLiteral("StubWidgets"));
	}

	/// Re-running the generator must never clobber the user's edits.
	void test_writeSkeleton_neverOverwritesExistingFiles()
	{
		QString dir = themeDir("precious");
		const QByteArray json = R"({"name":"Mine","widgets":"Fusion"})";
		const QByteArray css = "/* my own work */";
		QVERIFY(writeFile(dir + "/theme.json", json));
		QVERIFY(writeFile(dir + "/themeStyle.css", css));

		QVERIFY(CustomTheme::writeSkeleton(&m_base, "precious"));

		QCOMPARE(readFile(dir + "/theme.json"), json);
		QCOMPARE(readFile(dir + "/themeStyle.css"), css);
	}
};

QTEST_GUILESS_MAIN(CustomThemeTest)

#include "CustomTheme_test.moc"
