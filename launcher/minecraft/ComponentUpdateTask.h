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
