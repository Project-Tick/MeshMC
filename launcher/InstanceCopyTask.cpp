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

#include "InstanceCopyTask.h"
#include "settings/INISettingsObject.h"
#include "FileSystem.h"
#include "NullInstance.h"
#include "pathmatcher/RegexpMatcher.h"
#include <QtConcurrentRun>

InstanceCopyTask::InstanceCopyTask(InstancePtr origInstance, bool copySaves,
								   bool keepPlaytime)
{
	m_origInstance = origInstance;
	m_keepPlaytime = keepPlaytime;

	if (!copySaves) {
		// FIXME: get this from the original instance type...
		auto matcherReal = new RegexpMatcher("[.]?minecraft/saves");
		matcherReal->caseSensitive(false);
		m_matcher.reset(matcherReal);
	}
}

void InstanceCopyTask::executeTask()
{
	setStatus(tr("Copying instance %1").arg(m_origInstance->name()));

	FS::copy folderCopy(m_origInstance->instanceRoot(), m_stagingPath);
	folderCopy.followSymlinks(false).blacklist(m_matcher.get());

	m_copyFuture = QtConcurrent::run(QThreadPool::globalInstance(), folderCopy);
	connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::finished, this,
			&InstanceCopyTask::copyFinished);
	connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::canceled, this,
			&InstanceCopyTask::copyAborted);
	m_copyFutureWatcher.setFuture(m_copyFuture);
}

void InstanceCopyTask::copyFinished()
{
	auto successful = m_copyFuture.result();
	if (!successful) {
		emitFailed(tr("Instance folder copy failed."));
		return;
	}
	// FIXME: shouldn't this be able to report errors?
	auto instanceSettings = std::make_shared<INISettingsObject>(
		FS::PathCombine(m_stagingPath, "instance.cfg"));
	instanceSettings->registerSetting("InstanceType", "Legacy");

	InstancePtr inst(
		new NullInstance(m_globalSettings, instanceSettings, m_stagingPath));
	inst->setName(m_instName);
	inst->setIconKey(m_instIcon);
	if (!m_keepPlaytime) {
		inst->resetTimePlayed();
	}
	emitSucceeded();
}

void InstanceCopyTask::copyAborted()
{
	emitFailed(tr("Instance folder copy has been aborted."));
	return;
}
