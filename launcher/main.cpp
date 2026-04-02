/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "Application.h"

// #define BREAK_INFINITE_LOOP
// #define BREAK_EXCEPTION
// #define BREAK_RETURN

#ifdef BREAK_INFINITE_LOOP
#include <thread>
#include <chrono>
#endif

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
	Application app(argc, argv);

	switch (app.status()) {
		case Application::StartingUp:
		case Application::Initialized: {
			Q_INIT_RESOURCE(multimc);
			Q_INIT_RESOURCE(backgrounds);
			Q_INIT_RESOURCE(documents);
			Q_INIT_RESOURCE(meshmc);

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
			return app.exec();
		}
		case Application::Failed:
			return 1;
		case Application::Succeeded:
			return 0;
	}
}
