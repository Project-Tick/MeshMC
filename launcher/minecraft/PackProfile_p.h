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

#include "Component.h"
#include <map>
#include <QTimer>
#include <QList>
#include <QMap>

class MinecraftInstance;
using ComponentContainer = QList<ComponentPtr>;
using ComponentIndex = QMap<QString, ComponentPtr>;

struct PackProfileData {
	// the instance this belongs to
	MinecraftInstance* m_instance;

	// the launch profile (volatile, temporary thing created on demand)
	std::shared_ptr<LaunchProfile> m_profile;

	// version information migrated from instance.cfg file. Single use on
	// migration!
	std::map<QString, QString> m_oldConfigVersions;
	QString getOldConfigVersion(const QString& uid) const
	{
		const auto iter = m_oldConfigVersions.find(uid);
		if (iter != m_oldConfigVersions.cend()) {
			return (*iter).second;
		}
		return QString();
	}

	// persistent list of components and related machinery
	ComponentContainer components;
	ComponentIndex componentIndex;
	bool dirty = false;
	QTimer m_saveTimer;
	Task::Ptr m_updateTask;
	bool loaded = false;
	bool interactionDisabled = true;
};
