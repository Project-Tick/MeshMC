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
#include "PackManifest.h"

namespace Flame
{
	class FileResolvingTask : public Task
	{
		Q_OBJECT
	  public:
		explicit FileResolvingTask(
			shared_qobject_ptr<QNetworkAccessManager> network,
			Flame::Manifest& toProcess);
		virtual ~FileResolvingTask() {};

		const Flame::Manifest& getResults() const
		{
			return m_toProcess;
		}

	  protected:
		virtual void executeTask() override;

	  protected slots:
		void netJobFinished();

	  private: /* data */
		shared_qobject_ptr<QNetworkAccessManager> m_network;
		Flame::Manifest m_toProcess;
		QVector<QByteArray> results;
		NetJob::Ptr m_dljob;
	};
} // namespace Flame
