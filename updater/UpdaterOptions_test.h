/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 */

#pragma once

#include <QObject>
#include <QTest>

class UpdaterOptionsTest : public QObject
{
	Q_OBJECT

  private slots:
	// Quote stripping. The updater was reported as "does nothing at all" when
	// started from cmd.exe with single-quoted paths: cmd.exe passes the quotes
	// straight through, and every path derived from them then pointed at a
	// directory that could not exist.
	void tst_Unquote_data();
	void tst_Unquote();
	void tst_CleanPathArgument_data();
	void tst_CleanPathArgument();
	void tst_QuotedRootFromCmdShell();
	void tst_QuotedRootWithBackslashOnWindows();

	// Command line
	void tst_ParseMinimalPrepare();
	void tst_ParseApplyStage();
	void tst_ParseRejectsUnknownOption();
	void tst_ParseRejectsNonNumericWaitPid();
	void tst_ParseHelp();

	// Usability checks
	void tst_ValidateRejectsMissingRoot();
	void tst_ValidateRejectsNonExistentRoot();
	void tst_ValidateRejectsRootThatIsAFile();
	void tst_ValidateRejectsMissingUrl();
	void tst_ValidateRejectsNonHttpUrl();
	void tst_ValidateRejectsMissingExec();
	void tst_ValidateAcceptsAGoodPrepare();
	void tst_ValidateRejectsApplyWithoutSource();
	void tst_ValidateAcceptsAGoodApply();

	// The hand-off from the first stage to the second must survive a round
	// trip through the command line, or the second stage quietly gets the
	// wrong paths.
	void tst_ApplyArgumentsRoundTrip();
};
