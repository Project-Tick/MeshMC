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
