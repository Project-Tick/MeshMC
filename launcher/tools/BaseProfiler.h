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

#include "BaseExternalTool.h"
#include "QObjectPtr.h"

class BaseInstance;
class SettingsObject;
class LaunchTask;
class QProcess;

class BaseProfiler : public BaseExternalTool
{
	Q_OBJECT
  public:
	explicit BaseProfiler(SettingsObjectPtr settings, InstancePtr instance,
						  QObject* parent = 0);

  public slots:
	void beginProfiling(shared_qobject_ptr<LaunchTask> process);
	void abortProfiling();

  protected:
	QProcess* m_profilerProcess;

	virtual void beginProfilingImpl(shared_qobject_ptr<LaunchTask> process) = 0;
	virtual void abortProfilingImpl();

  signals:
	void readyToLaunch(const QString& message);
	void abortLaunch(const QString& message);
};

class BaseProfilerFactory : public BaseExternalToolFactory
{
  public:
	virtual BaseProfiler* createProfiler(InstancePtr instance,
										 QObject* parent = 0);
};
