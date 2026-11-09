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

#include <QSignalSpy>

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QToolBar>

#include <KLocalizedString>
#include <KMainWindow>
#include <KSharedConfig>

#include "settings/KConfigSettingsObject.h"
#include "ui/WindowState.h"

namespace
{
	// saveAutoSaveSettings() is protected: this is what closeEvent() calls.
	class TestWindow : public KMainWindow
	{
	  public:
		TestWindow()
		{
			setAttribute(Qt::WA_DeleteOnClose, false);
			auto* tools = new QToolBar(QStringLiteral("tools"), this);
			tools->setObjectName(QStringLiteral("tools")); // saveState() wants it
			addToolBar(tools);
			setCentralWidget(new QWidget(this));
		}
		void save()
		{
			saveAutoSaveSettings();
		}
	};

	QByteArray slurp(const QString& path)
	{
		QFile file(path);
		return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
	}
} // namespace

// How the main window and the console window come back the way they were.
class WindowStateTest : public QObject
{
	Q_OBJECT

	// the legacy file of a window that had the given size, in the form
	// earlier versions wrote it
	static QString legacyFile(const QTemporaryDir& dir, const QSize& size)
	{
		TestWindow window;
		window.resize(size);
		window.show();
		const QString path = dir.filePath("meshmc.cfg");
		QFile file(path);
		file.open(QIODevice::WriteOnly);
		file.write("MainWindowGeometry=" + window.saveGeometry().toBase64() +
				   "\nMainWindowState=" + window.saveState().toBase64() +
				   "\nInstanceDir=inst\n");
		return path;
	}

  private slots:
	// The state of the windows of the test is kept like the real one is, so it
	// has to be gone before and after, or a second run starts from the first.
	static void forgetTheWindows()
	{
		auto config = KSharedConfig::openStateConfig();
		for (const char* group :
			 {"SavedWindow", "LegacyWindow", "OwnWindow", "KindA", "KindB",
			  "QuitWindow"}) {
			config->deleteGroup(QString::fromLatin1(group));
		}
		config->sync();
	}

	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
		KLocalizedString::setApplicationDomain("meshmc");
		forgetTheWindows();
	}

	void cleanupTestCase()
	{
		forgetTheWindows();
	}

	void test_AWindowComesBackTheWayItWasSaved()
	{
		const QString group = QStringLiteral("SavedWindow");
		{
			TestWindow window;
			WindowState::setup(&window, group, nullptr, QString(), QString());
			window.resize(512, 384);
			window.show();
			window.save();
		}

		TestWindow again;
		WindowState::setup(&again, group, nullptr, QString(), QString());
		QCOMPARE(again.size(), QSize(512, 384));
	}

	void test_TheSettingsOfEarlierVersionsAreTakenOnceAndRemoved()
	{
		QTemporaryDir dir;
		const QString path = legacyFile(dir, QSize(600, 420));
		KConfigSettingsObject legacy(path);

		TestWindow window;
		WindowState::setup(&window, QStringLiteral("LegacyWindow"), &legacy,
						   QStringLiteral("MainWindowGeometry"),
						   QStringLiteral("MainWindowState"));
		QCOMPARE(window.size(), QSize(600, 420));

		const QByteArray written = slurp(path);
		QVERIFY(!written.contains("MainWindowGeometry"));
		QVERIFY(!written.contains("MainWindowState"));
		QVERIFY(written.contains("InstanceDir=inst"));
	}

	void test_WhatTheWindowSavedItselfWinsOverTheOldSettings()
	{
		const QString group = QStringLiteral("OwnWindow");
		{
			TestWindow window;
			WindowState::setup(&window, group, nullptr, QString(), QString());
			window.resize(500, 400);
			window.show();
			window.save();
		}

		QTemporaryDir dir;
		const QString path = legacyFile(dir, QSize(900, 700));
		KConfigSettingsObject legacy(path);

		TestWindow window;
		WindowState::setup(&window, group, &legacy,
						   QStringLiteral("MainWindowGeometry"),
						   QStringLiteral("MainWindowState"));
		QCOMPARE(window.size(), QSize(500, 400));
		// and they are gone all the same
		QVERIFY(!slurp(path).contains("MainWindowGeometry"));
	}

	void test_EveryKindOfWindowHasItsOwn()
	{
		{
			TestWindow main;
			WindowState::setup(&main, QStringLiteral("KindA"), nullptr,
							   QString(), QString());
			main.resize(500, 400);
			main.show();
			main.save();
		}

		// the console window does not start like the main window
		TestWindow console;
		WindowState::setup(&console, QStringLiteral("KindB"), nullptr,
						   QString(), QString());
		QVERIFY(console.size() != QSize(500, 400));
	}

	void test_ClosingDoesNotQuitTheApplication()
	{
		// KMainWindow::closeEvent() would; the windows of MeshMC do not call
		// it. This is the one thing the application depends on that a window
		// could get wrong by being a KMainWindow.
		QSignalSpy quit(qApp, &QCoreApplication::aboutToQuit);
		{
			TestWindow window;
			WindowState::setup(&window, QStringLiteral("QuitWindow"), nullptr,
							   QString(), QString());
			window.show();
			window.save();
			window.close();
		}
		QTest::qWait(50);
		QCOMPARE(quit.count(), 0);
	}
};

QTEST_MAIN(WindowStateTest)

#include "WindowState_test.moc"
