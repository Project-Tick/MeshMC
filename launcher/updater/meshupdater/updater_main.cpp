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

#include "MeshUpdaterApp.h"

int main(int argc, char* argv[])
{
	// Answered before a QApplication exists, because a probe has to work
	// where one cannot be constructed: without a display, constructing
	// QApplication aborts, and "the updater cannot start" would then be
	// indistinguishable from "there is no screen".
	if (MeshUpdaterApp::handleSelfTest(argc, argv))
		return MeshUpdaterApp::ExitSuccess;

	MeshUpdaterApp app(argc, argv);

	switch (app.status()) {
		case MeshUpdaterApp::Starting:
		case MeshUpdaterApp::Initialized:
			// The work is queued on the event loop, so the exit code comes
			// from whatever the app passes to QCoreApplication::exit().
			return app.exec();

		case MeshUpdaterApp::Succeeded:
			return MeshUpdaterApp::ExitSuccess;

		case MeshUpdaterApp::Failed:
			return MeshUpdaterApp::ExitFailed;

		case MeshUpdaterApp::Aborted:
			return MeshUpdaterApp::ExitAborted;
	}

	// Unreachable unless a new Status is added without being handled here,
	// which must not silently look like success.
	return MeshUpdaterApp::ExitFailed;
}
