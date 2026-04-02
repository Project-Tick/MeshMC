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
#include "minecraft/VersionFilterData.h"

class MinecraftInstance;

class FMLLibrariesTask : public Task
{
	Q_OBJECT
  public:
	FMLLibrariesTask(MinecraftInstance* inst);
	virtual ~FMLLibrariesTask() {};

	void executeTask() override;

	bool canAbort() const override;

  private slots:
	void fmllibsFinished();
	void fmllibsFailed(QString reason);

  public slots:
	bool abort() override;

  private:
	MinecraftInstance* m_inst;
	NetJob::Ptr downloadJob;
	QList<FMLlib> fmlLibsToProcess;
};
