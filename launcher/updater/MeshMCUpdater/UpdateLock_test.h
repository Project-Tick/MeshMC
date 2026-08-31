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

class UpdateLockTest : public QObject
{
	Q_OBJECT

  private slots:
	// Claiming
	void tst_ClaimOnCleanDirectory();
	void tst_ClaimRefusedWhileOwnerIsAlive();
	void tst_ClaimTakesOverALockWhoseOwnerIsGone();
	void tst_ClaimReportsWhatItTookOver();
	void tst_SetStageKeepsTheOriginalStartTime();
	void tst_ReleaseRemovesTheFile();
	void tst_PeekOnMissingFile();

	// The hand-off signal. The first stage cannot tell "I started the second
	// stage" from "the second stage actually ran" any other way, and getting
	// this wrong is what made a failed update look exactly like a successful
	// one: the launcher closed and nothing happened.
	void tst_TakeoverAcceptedWhenAnotherProcessClaimsIt();
	void tst_TakeoverAcceptedWhenItHappensMidWait();
	void tst_TakeoverReleasedWhenUpdateAlreadyFinished();
	void tst_TakeoverOwnerGoneWhenChildDiedWithoutClaiming();
	void tst_TakeoverPrefersClaimOverDeadChild();
	void tst_TakeoverTimesOutWhileChildIsStillAlive();
	void tst_DescribeTakeoverCoversEveryOutcome();
};
