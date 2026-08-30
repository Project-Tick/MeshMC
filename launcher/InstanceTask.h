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

#include "tasks/Task.h"
#include "settings/SettingsObject.h"

class InstanceTask : public Task
{
	Q_OBJECT
  public:
	explicit InstanceTask();
	virtual ~InstanceTask();

	void setParentSettings(SettingsObjectPtr settings)
	{
		m_globalSettings = settings;
	}

	void setStagingPath(const QString& stagingPath)
	{
		m_stagingPath = stagingPath;
	}

	void setName(const QString& name)
	{
		m_instName = name;
	}
	QString name() const
	{
		return m_instName;
	}

	void setIcon(const QString& icon)
	{
		m_instIcon = icon;
	}

	void setGroup(const QString& group)
	{
		m_instGroup = group;
	}
	QString group() const
	{
		return m_instGroup;
	}

	/* ---- Overwriting an existing instance ---------------------------
	 *
	 * By default a task stages a new instance directory and the staging
	 * wrapper moves it into place under a fresh id. Updating a modpack
	 * in place is the same work with a different ending: the staged
	 * directory replaces an existing instance instead of becoming a new
	 * one, so that the instance keeps its id, its group, its saves and
	 * everything else that refers to it by id (shortcuts, the group
	 * index, the "last launched" bookkeeping).
	 *
	 * Kept on InstanceTask rather than on the importer because it is the
	 * *staging* step that has to behave differently, and that step only
	 * ever sees an InstanceTask.
	 */

	/* Replace `instanceId` with whatever this task stages. Passing an
	 * empty id turns overriding back off. */
	void setOverrideInstance(const QString& instanceId)
	{
		m_overrideInstanceId = instanceId;
	}

	/* An empty id is the whole "no" answer: there is no way to override
	 * without naming a target, so a separate flag could only ever
	 * disagree with this. */
	bool shouldOverride() const
	{
		return !m_overrideInstanceId.isEmpty();
	}

	QString overrideInstanceId() const
	{
		return m_overrideInstanceId;
	}

	/* ---- Files the update makes obsolete ----------------------------
	 *
	 * When a pack update drops a mod, merging the new version over the
	 * old one does not remove it: the merge only replaces what the new
	 * version actually ships, which is the whole point - it is what
	 * keeps the user's worlds and configs. So a file the pack no longer
	 * carries has to be named explicitly, or it lingers and gets loaded
	 * alongside the version that replaced it.
	 *
	 * Collected as absolute paths by whoever worked out that they are
	 * obsolete, and acted on by the staging step *after* the merge
	 * succeeds. Deleting earlier would mean a failed or aborted update
	 * leaves the instance missing mods it still needs.
	 */
	void scheduleForRemoval(const QString& absolutePath)
	{
		if (!absolutePath.isEmpty() &&
			!m_filesToRemoveAfterCommit.contains(absolutePath)) {
			m_filesToRemoveAfterCommit.append(absolutePath);
		}
	}

	QStringList filesToRemoveAfterCommit() const
	{
		return m_filesToRemoveAfterCommit;
	}

  protected: /* data */
	SettingsObjectPtr m_globalSettings;
	QString m_instName;
	QString m_instIcon;
	QString m_instGroup;
	QString m_stagingPath;
	QString m_overrideInstanceId;
	QStringList m_filesToRemoveAfterCommit;
};
