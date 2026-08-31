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
