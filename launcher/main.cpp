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

#include "Application.h"
#include "settings/GlobalSettings.h"
#include "BuildConfig.h"

#include <QProcessEnvironment>
#include <KLocalizedString>
#include <KAboutData>
#include <KCrash>
#include <QFile>

#include <cstdlib>
#ifndef Q_OS_WIN
#include <unistd.h>
#endif

// #define BREAK_INFINITE_LOOP
// #define BREAK_EXCEPTION
// #define BREAK_RETURN

#ifdef BREAK_INFINITE_LOOP
#include <thread>
#include <chrono>
#endif

// Called by KCrash from its signal handler, before DrKonqi is started. There
// is no crash dialog of our own: the log file is what a crash leaves behind.
static void flushLogOnCrash(int)
{
	if (APPLICATION && APPLICATION->logFile) {
		APPLICATION->logFile->flush();
	}
}

int main(int argc, char* argv[])
{
#ifdef BREAK_INFINITE_LOOP
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
	}
#endif
#ifdef BREAK_EXCEPTION
	throw 42;
#endif
#ifdef BREAK_RETURN
	return 42;
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(6, 8, 0))
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

	// initialize Qt
#ifdef Q_OS_LINUX
	{
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

		// Prefer Wayland backend only when a Wayland socket is actually
		// accessible. WAYLAND_DISPLAY is set by the compositor/Flatpak
		// only when the socket is forwarded; XDG_SESSION_TYPE alone is
		// not sufficient because Flatpak sandboxes may report "wayland"
		// there even when --socket=wayland is not granted.
		if (!env.contains("QT_QPA_PLATFORM") &&
			env.contains("WAYLAND_DISPLAY")) {
			qputenv("QT_QPA_PLATFORM", "wayland");
		}

		// Use xdgdesktopportal for file dialogs and theming on all
		// Linux DEs.  The old "gtk3" theme forced Qt to use GTK3's
		// native file-dialog integration, which deadlocks/freezes
		// on GNOME/Mutter (and other GTK-based compositors) with
		// Qt 6.  xdgdesktopportal delegates to the DE's own portal
		// implementation and works everywhere.
		if (!env.contains("QT_QPA_PLATFORMTHEME")) {
			qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
		}
	}
#endif

	QCoreApplication::setApplicationName(QStringLiteral("MeshMC"));

	KLocalizedString::setApplicationDomain("meshmc");

	Application app(argc, argv);

	KAboutData aboutData(
		QStringLiteral("MeshMC"),
		i18nc("@title", "MeshMC"),
		BuildConfig.printableVersionString(),
		i18nc("@info", "A custom launcher that makes managing Minecraft easier by allowing you to have multiple instances of Minecraft at once."),
		KAboutLicense::Apache_V2,
		i18nc("@info:credit", "Copyright 2013–2021 MultiMC Contributors\n"
			  "Copyright © 2025-%1 Project Tick", QStringLiteral("2026")));

	aboutData.setHomepage(QStringLiteral("https://projecttick.org"));
	aboutData.addAuthor(i18nc("@info:credit", "Mehmet Samet Duman"),
						i18nc("@info:credit", "Maintainer (since 2025) and developer"),
						QStringLiteral("yongdohyun@projecttick.org"));
	aboutData.addAuthor(i18nc("@info:credit", "Andrew Okin"),
						i18nc("@info:credit", "The creator of MultiMC"),
						QStringLiteral("forkk@forkk.net"));
	aboutData.addAuthor(i18nc("@info:credit", "Petr Mrázek"),
						i18nc("@info:credit", "MultiMC maintainer"),
						QStringLiteral("peterix@gmail.com"));
	aboutData.addAuthor(i18nc("@info:credit", "Sky Welch"),
						i18nc("@info:credit", "MultiMC maintainer"),
						QStringLiteral("multimc@bunnies.io"));
	aboutData.addAuthor(i18nc("@info:credit", "Jan (02JanDal)"),
						i18nc("@info:credit", "MultiMC maintainer"),
						QStringLiteral("02jandal@gmail.com"));
	aboutData.addAuthor(i18nc("@info:credit", "RoboSky"),
						i18nc("@info:credit", "MultiMC maintainer"));
	aboutData.setOrganizationDomain("projecttick.org");
	aboutData.setDesktopFileName("org.projecttick.MeshMC");

	aboutData.setProgramLogo(APPLICATION->getThemedIcon("logo"));

	aboutData.setBugAddress(BuildConfig.BUG_TRACKER_URL.toUtf8());

	KAboutData::setApplicationData(aboutData);

	// Crash handling is KCrash's: it catches the fatal signals, flushes the
	// log, and starts DrKonqi where that is installed (a KDE desktop). Where
	// it is not, the process just ends the way it would have. Must come after
	// the KAboutData is set, which DrKonqi reports from.
	KCrash::initialize();
	KCrash::setDrKonqiEnabled(true);
	KCrash::setEmergencySaveFunction(flushLogOnCrash);

	switch (app.status()) {
		case Application::StartingUp:
		case Application::Initialized: {
			Q_INIT_RESOURCE(multimc);
			Q_INIT_RESOURCE(backgrounds);
			Q_INIT_RESOURCE(documents);
			Q_INIT_RESOURCE(meshmc);
			Q_INIT_RESOURCE(shaders);

			Q_INIT_RESOURCE(pe_dark);
			Q_INIT_RESOURCE(pe_light);
			Q_INIT_RESOURCE(pe_blue);
			Q_INIT_RESOURCE(pe_colored);
			Q_INIT_RESOURCE(breeze_dark);
			Q_INIT_RESOURCE(breeze_light);
			Q_INIT_RESOURCE(OSX);
			Q_INIT_RESOURCE(iOS);
			Q_INIT_RESOURCE(flat);
			Q_INIT_RESOURCE(flat_white);

			int ret = app.exec();

			// Use _exit() to terminate immediately after the Qt event
			// loop ends.  All meaningful cleanup (instance save, plugin
			// shutdown, log flush/close) has already happened in the
			// Application::aboutToQuit handler while Qt was still alive.
			//
			// A normal return would run C++ static destructors via
			// exit()/__cxa_finalize.  Plugin .mmco modules statically
			// link MeshMC_logic, which embeds a duplicate global
			// `const Config BuildConfig` (non-trivial dtor with ~20
			// QString members).  Those duplicate destructors corrupt
			// the heap when they run after Qt is torn down, causing
			// glibc's "corrupted double-linked list" abort.
			//
			// _exit() bypasses all atexit handlers and static dtors,
			// avoiding the double-destruction entirely.  The OS
			// reclaims all process memory.
			_exit(ret);
		}
		case Application::Failed:
			return 1;
		case Application::Succeeded:
			return 0;
	}
}
