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

#include "qml/Theme/KirigamiColors.h"
#include "qml/Theme/ThemeService.h"
#include "theme/SystemPalette.h"
#include "theme/ThemePalette.h"

#include <QtTest>
#include <QColor>
#include <QGuiApplication>
#include <QPalette>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QUrl>

#include <memory>

namespace
{
	/* A stand-in for the desktop's Kirigami plugin: Breeze's dark colours
	 * per colour set, set the way a real platform theme sets them. */
	class FakeBreezeTheme : public Kirigami::Platform::PlatformTheme
	{
	  public:
		FakeBreezeTheme(ColorSet set, QObject* parent) : PlatformTheme(parent)
		{
			setInherit(false);
			setColorSet(set);
			switch (set) {
				case Window:
					setBackgroundColor(QColor(0x20, 0x23, 0x26));
					setTextColor(QColor(0xFC, 0xFC, 0xFC));
					setDisabledTextColor(QColor(0x6E, 0x71, 0x75));
					setHighlightColor(QColor(0x3D, 0xAE, 0xE9));
					setHighlightedTextColor(QColor(0xFC, 0xFC, 0xFC));
					setPositiveTextColor(QColor(0x27, 0xAE, 0x60));
					setNeutralTextColor(QColor(0xF6, 0x74, 0x00));
					setNegativeTextColor(QColor(0xDA, 0x44, 0x53));
					setActiveTextColor(QColor(0x3D, 0xAE, 0xE9));
					break;
				case View:
					setBackgroundColor(QColor(0x14, 0x16, 0x18));
					setDisabledTextColor(QColor(0x6E, 0x71, 0x75));
					setFocusColor(QColor(0x3D, 0xAE, 0xE9));
					break;
				case Selection:
					setBackgroundColor(QColor(0x3D, 0xAE, 0xE9));
					setTextColor(QColor(0xFC, 0xFC, 0xFC));
					break;
				case Tooltip:
					setBackgroundColor(QColor(0x29, 0x2C, 0x30));
					setTextColor(QColor(0xFC, 0xFC, 0xFC));
					break;
				default:
					break;
			}
		}

		// The user picking another accent colour.
		void pickAccent(const QColor& accent) { setHighlightColor(accent); }
	};
} // namespace

/*
 * Smoke test for MeshMC.Theme, the same job QmlModule_test.cpp does for the
 * MeshMC module: proves the module resolves at its resource prefix, the
 * singleton instantiates, and the tokens it hands out are the ones this
 * module declares -- not that any UI built on top of them looks right.
 *
 * The component is supplied via setData() rather than a loadFromModule()/qrc
 * URL, because it only needs to *use* the module, not live inside it; a
 * plain QUrl base is enough for relative resolution and needs no resource
 * entry of its own.
 */
class ThemeTest : public QObject
{
	Q_OBJECT

  private slots:
	void tokensAndModeSwitch()
	{
		QQmlEngine engine;
		engine.addImportPath(QStringLiteral("qrc:/qt/qml"));

		QQmlComponent component(&engine);
		component.setData(QByteArrayLiteral(
							   "import QtQuick\n"
							   "import MeshMC.Theme\n"
							   "QtObject {\n"
							   "    property color accent: Theme.palette.accent\n"
							   "    property int spaceMd: Theme.space.md\n"
							   "    property int bodyPixelSize: Theme.type.body.pixelSize\n"
							   "    property int radiusMd: Theme.radius.md\n"
							   "    property bool dark: Theme.dark\n"
							   "    property color canvasColor: Theme.palette.canvas\n"
							   "    // Exposed so the test can reach the singletons from C++\n"
							   "    // without QQmlEngine::singletonInstance(uri, typeName),\n"
							   "    // which is Qt 6.5+ and this project's floor is 6.4.\n"
							   "    property QtObject themeRef: Theme\n"
							   "    property QtObject serviceRef: ThemeService\n"
							   "}\n"),
						   QUrl(QStringLiteral("inline")));

		QVERIFY2(!component.isError(), qPrintable(component.errorString()));

		std::unique_ptr<QObject> root(component.create());
		QVERIFY2(root != nullptr, qPrintable(component.errorString()));

		// The plain constants: exactly the values Theme.qml declares.
		QCOMPARE(root->property("spaceMd").toInt(), 12);
		QCOMPARE(root->property("bodyPixelSize").toInt(), 14);
		QCOMPARE(root->property("radiusMd").toInt(), 6);

		auto* service = qobject_cast<ThemeService*>(
			root->property("serviceRef").value<QObject*>());
		QVERIFY(service != nullptr);
		QObject* themeRef = root->property("themeRef").value<QObject*>();
		QVERIFY(themeRef != nullptr);

		// The colour tokens: not merely "some colour", but the one
		// ThemePalette hands out for whichever theme is active -- proving
		// the token came from ThemePalette rather than a value made up here.
		//
		// A fresh ThemeService starts on the "system" scheme, the app's
		// default (see its own m_followSystem), in the desktop's own colours
		// and light or dark with them -- here the offscreen platform's light
		// QPalette, which is what this runner's "desktop" is.
		const SystemColors desktop = SystemColors::fromPalette(QGuiApplication::palette());
		QCOMPARE(service->scheme(), QStringLiteral("system"));
		QCOMPARE(root->property("dark").toBool(), desktop.isDark());
		QCOMPARE(root->property("accent").value<QColor>(),
				 ThemePalette::fromSystem(desktop).accent);

		// The named schemes start dark whatever the OS prefers -- that is
		// the documented default mode -- so this holds on a light runner too.
		// Grass is checked rather than meshDark() (Amethyst) on purpose, so
		// it is Grass's own palette arriving, not whichever one is first.
		QSignalSpy toGrass(service, &ThemeService::changed);
		themeRef->setProperty("scheme", QStringLiteral("grass"));
		QCOMPARE(toGrass.count(), 1);
		QCOMPARE(root->property("dark").toBool(), true);
		QCOMPARE(root->property("accent").value<QColor>(),
				 ThemePalette::forScheme(ThemePalette::Scheme::Grass, true).accent);

		// Switching mode replaces the whole theme as one unit: `dark` and
		// `palette` land on the new theme together, and `changed` fires
		// exactly once per switch, never once per property.
		QSignalSpy toLight(service, &ThemeService::changed);
		themeRef->setProperty("mode", QStringLiteral("light"));
		QCOMPARE(toLight.count(), 1);
		QCOMPARE(root->property("dark").toBool(), false);
		QCOMPARE(root->property("canvasColor").value<QColor>(),
				 ThemePalette::forScheme(ThemePalette::Scheme::Grass, false).canvas);

		QSignalSpy toDark(service, &ThemeService::changed);
		themeRef->setProperty("mode", QStringLiteral("dark"));
		QCOMPARE(toDark.count(), 1);
		QCOMPARE(root->property("dark").toBool(), true);
		QCOMPARE(root->property("canvasColor").value<QColor>(),
				 ThemePalette::forScheme(ThemePalette::Scheme::Grass, true).canvas);

		// The colour scheme swaps independently of the mode, also as one
		// announced unit; an unknown name changes nothing.
		QSignalSpy toEmber(service, &ThemeService::changed);
		themeRef->setProperty("scheme", QStringLiteral("ember"));
		QCOMPARE(toEmber.count(), 1);
		QCOMPARE(root->property("accent").value<QColor>(),
				 ThemePalette::forScheme(ThemePalette::Scheme::Ember, true).accent);
		QCOMPARE(root->property("dark").toBool(), true);

		QSignalSpy ignored(service, &ThemeService::changed);
		service->setScheme(QStringLiteral("nonsense"));
		QCOMPARE(ignored.count(), 0);
		QCOMPARE(service->scheme(), QStringLiteral("ember"));
		QCOMPARE(service->previewPalette(QStringLiteral("diamond")).accent,
				 ThemePalette::forScheme(ThemePalette::Scheme::Diamond, true).accent);
		QCOMPARE(service->previewPalette(QStringLiteral("system")),
				 service->systemPalette());
	}

	/// The "system" scheme follows the desktop live: a new application
	/// palette (QEvent::ApplicationPaletteChange) is re-read at once, light
	/// or dark included, and announced as one change.
	void systemSchemeFollowsPaletteChanges()
	{
		ThemeService service;
		QCOMPARE(service.scheme(), QStringLiteral("system"));
		const QPalette original = QGuiApplication::palette();
		const bool wasDark = service.dark();

		QPalette flipped = original;
		flipped.setColor(QPalette::Window, wasDark ? QColor(0xEF, 0xF0, 0xF1) : QColor(0x20, 0x23, 0x26));
		flipped.setColor(QPalette::Base, wasDark ? QColor(0xFF, 0xFF, 0xFF) : QColor(0x14, 0x16, 0x18));
		flipped.setColor(QPalette::WindowText, wasDark ? QColor(0x23, 0x26, 0x29) : QColor(0xFC, 0xFC, 0xFC));

		QSignalSpy changed(&service, &ThemeService::changed);
		QSignalSpy systemChanged(&service, &ThemeService::systemPaletteChanged);
		QGuiApplication::setPalette(flipped);
		QCOMPARE(systemChanged.count(), 1);
		QCOMPARE(changed.count(), 1);
		QCOMPARE(service.dark(), !wasDark);
		QCOMPARE(service.palette(), ThemePalette::fromSystem(SystemColors::fromPalette(flipped)));

		// A named scheme in a fixed mode ignores the desktop, but the System
		// card's preview still follows it.
		service.setScheme(QStringLiteral("ember"));
		QSignalSpy ignored(&service, &ThemeService::changed);
		QGuiApplication::setPalette(original);
		QCOMPARE(ignored.count(), 0);
		QCOMPARE(service.systemPalette(),
				 ThemePalette::fromSystem(SystemColors::fromPalette(QGuiApplication::palette())));
	}

	/// On Plasma the colours come from Kirigami's colour sets, each read
	/// from the set a KDE app would draw that part in, and a change to any
	/// of them is announced once.
	void kirigamiColorSets()
	{
		using PlatformTheme = Kirigami::Platform::PlatformTheme;
		FakeBreezeTheme* window = nullptr;
		KirigamiColors colors([&window](PlatformTheme::ColorSet set, QObject* parent) {
			auto* theme = new FakeBreezeTheme(set, parent);
			if (set == PlatformTheme::Window)
				window = theme;
			return theme;
		});
		QVERIFY(colors.isValid());
		QVERIFY(window != nullptr);

		const SystemColors s = colors.colors();
		QCOMPARE(s.window, QColor(0x20, 0x23, 0x26));
		QCOMPARE(s.view, QColor(0x14, 0x16, 0x18));
		QCOMPARE(s.text, QColor(0xFC, 0xFC, 0xFC));
		QCOMPARE(s.accent, QColor(0x3D, 0xAE, 0xE9));
		QCOMPARE(s.highlight, QColor(0x3D, 0xAE, 0xE9));
		QCOMPARE(s.tooltip, QColor(0x29, 0x2C, 0x30));
		QCOMPARE(s.positive, QColor(0x27, 0xAE, 0x60));
		QCOMPARE(s.negative, QColor(0xDA, 0x44, 0x53));
		QCOMPARE(s.focus, QColor(0x3D, 0xAE, 0xE9));
		QVERIFY(s.isDark());

		QSignalSpy changed(&colors, &KirigamiColors::changed);
		window->pickAccent(QColor(0xE9, 0x3D, 0x82));
		QTRY_COMPARE(changed.count(), 1);
		QCOMPARE(colors.colors().accent, QColor(0xE9, 0x3D, 0x82));
	}
};

int main(int argc, char* argv[])
{
	/* Qt Quick types need a QGuiApplication, and ThemeService itself reads
	 * QGuiApplication::palette() -- QTEST_GUILESS_MAIN (QCoreApplication),
	 * what most other tests in this tree use, is not an option here. Forcing
	 * the offscreen platform keeps it runnable on a headless CI runner
	 * without depending on the harness to set QT_QPA_PLATFORM for us. */
	qputenv("QT_QPA_PLATFORM", "offscreen");

	QGuiApplication app(argc, argv);
	ThemeTest test;
	return QTest::qExec(&test, argc, argv);
}

#include "Theme_test.moc"
