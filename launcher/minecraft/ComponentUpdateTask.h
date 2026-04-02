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
#include "net/Mode.h"

#include <memory>
class PackProfile;
struct ComponentUpdateTaskData;

class ComponentUpdateTask : public Task
{
	Q_OBJECT
  public:
	enum class Mode { Launch, Resolution };

  public:
	explicit ComponentUpdateTask(Mode mode, Net::Mode netmode,
								 PackProfile* list, QObject* parent = 0);
	virtual ~ComponentUpdateTask();

  protected:
	void executeTask();

  private:
	void loadComponents();
	void resolveDependencies(bool checkOnly);

	void remoteLoadSucceeded(size_t index);
	void remoteLoadFailed(size_t index, const QString& msg);
	void checkIfAllFinished();

  private:
	std::unique_ptr<ComponentUpdateTaskData> d;
};
