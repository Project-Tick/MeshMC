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

#pragma once

#include "tasks/Task.h"
#include "net/NetJob.h"
#include <QUrl>
#include <QFuture>
#include <QFutureWatcher>
#include "settings/SettingsObject.h"
#include "BaseVersion.h"
#include "BaseInstance.h"
#include "InstanceTask.h"

class InstanceCopyTask : public InstanceTask
{
	Q_OBJECT
  public:
	explicit InstanceCopyTask(InstancePtr origInstance, bool copySaves,
							  bool keepPlaytime);

  protected:
	//! Entry point for tasks.
	virtual void executeTask() override;
	void copyFinished();
	void copyAborted();

  private: /* data */
	InstancePtr m_origInstance;
	QFuture<bool> m_copyFuture;
	QFutureWatcher<bool> m_copyFutureWatcher;
	std::unique_ptr<IPathMatcher> m_matcher;
	bool m_keepPlaytime;
};
