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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include <tasks/Task.h>

#include <QString>
#include <QJsonObject>
#include <QTimer>
#include <qsslerror.h>

#include "MinecraftAccount.h"

class QNetworkReply;

/**
 * Enum for describing the state of the current task.
 * Used by the getStateMessage function to determine what the status message
 * should be.
 */
enum class AccountTaskState {
	STATE_CREATED,
	STATE_WORKING,
	STATE_SUCCEEDED,
	STATE_FAILED_SOFT, //!< soft failure. authentication went through partially
	STATE_FAILED_HARD, //!< hard failure. main tokens are invalid
	STATE_FAILED_GONE, //!< hard failure. main tokens are invalid, and the
					   //!< account no longer exists
	STATE_OFFLINE //!< soft failure. authentication failed in the first step in
				  //!< a 'soft' way
};

class AccountTask : public Task
{
	Q_OBJECT
  public:
	explicit AccountTask(AccountData* data, QObject* parent = 0);
	virtual ~AccountTask() {};

	AccountTaskState m_taskState = AccountTaskState::STATE_CREATED;

	AccountTaskState taskState()
	{
		return m_taskState;
	}

  signals:
	void authorizeWithBrowser(const QUrl& url);

  protected:
	/**
	 * Returns the state message for the given state.
	 * Used to set the status message for the task.
	 * Should be overridden by subclasses that want to change messages for a
	 * given state.
	 */
	virtual QString getStateMessage() const;

  protected slots:
	// NOTE: true -> non-terminal state, false -> terminal state
	bool changeState(AccountTaskState newState, QString reason = QString());

  protected:
	AccountData* m_data = nullptr;
};
