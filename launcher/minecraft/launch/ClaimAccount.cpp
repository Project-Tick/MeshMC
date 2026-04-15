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

#include "ClaimAccount.h"
#include <launch/LaunchTask.h>

#include "Application.h"
#include "minecraft/auth/AccountList.h"

ClaimAccount::ClaimAccount(LaunchTask* parent, AuthSessionPtr session)
	: LaunchStep(parent)
{
	if (session->status == AuthSession::Status::PlayableOnline &&
		!session->demo) {
		auto accounts = APPLICATION->accounts();
		m_account = accounts->getAccountByProfileName(session->player_name);
	}
}

void ClaimAccount::executeTask()
{
	if (m_account) {
		lock.reset(new UseLock(m_account));
		emitSucceeded();
	}
}

void ClaimAccount::finalize()
{
	lock.reset();
}
