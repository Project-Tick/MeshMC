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

#include "BaseExternalTool.h"

#include <QProcess>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "BaseInstance.h"

BaseExternalTool::BaseExternalTool(SettingsObjectPtr settings,
								   InstancePtr instance, QObject* parent)
	: QObject(parent), m_instance(instance), globalSettings(settings)
{
}

BaseExternalTool::~BaseExternalTool() {}

BaseDetachedTool::BaseDetachedTool(SettingsObjectPtr settings,
								   InstancePtr instance, QObject* parent)
	: BaseExternalTool(settings, instance, parent)
{
}

void BaseDetachedTool::run()
{
	runImpl();
}

BaseExternalToolFactory::~BaseExternalToolFactory() {}

BaseDetachedTool*
BaseDetachedToolFactory::createDetachedTool(InstancePtr instance,
											QObject* parent)
{
	return qobject_cast<BaseDetachedTool*>(createTool(instance, parent));
}
