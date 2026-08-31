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

#include "BaseProfiler.h"
#include "QObjectPtr.h"

#include <QProcess>

BaseProfiler::BaseProfiler(SettingsObjectPtr settings, InstancePtr instance,
						   QObject* parent)
	: BaseExternalTool(settings, instance, parent)
{
}

void BaseProfiler::beginProfiling(shared_qobject_ptr<LaunchTask> process)
{
	beginProfilingImpl(process);
}

void BaseProfiler::abortProfiling()
{
	abortProfilingImpl();
}

void BaseProfiler::abortProfilingImpl()
{
	if (!m_profilerProcess) {
		return;
	}
	m_profilerProcess->terminate();
	m_profilerProcess->deleteLater();
	m_profilerProcess = 0;
	emit abortLaunch(tr("Profiler aborted"));
}

BaseProfiler* BaseProfilerFactory::createProfiler(InstancePtr instance,
												  QObject* parent)
{
	return qobject_cast<BaseProfiler*>(createTool(instance, parent));
}
