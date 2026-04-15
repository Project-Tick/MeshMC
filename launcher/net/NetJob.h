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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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
#include <QtNetwork>
#include "NetAction.h"
#include "Download.h"
#include "HttpMetaCache.h"
#include "tasks/Task.h"
#include "QObjectPtr.h"

class NetJob;

class NetJob : public Task
{
	Q_OBJECT
  public:
	using Ptr = shared_qobject_ptr<NetJob>;

	explicit NetJob(QString job_name,
					shared_qobject_ptr<QNetworkAccessManager> network)
		: Task(), m_network(network)
	{
		setObjectName(job_name);
	}
	virtual ~NetJob();

	bool addNetAction(NetAction::Ptr action);

	NetAction::Ptr operator[](int index)
	{
		return downloads[index];
	}
	const NetAction::Ptr at(const int index)
	{
		return downloads.at(index);
	}
	NetAction::Ptr first()
	{
		if (downloads.size())
			return downloads[0];
		return NetAction::Ptr();
	}
	int size() const
	{
		return downloads.size();
	}
	QStringList getFailedFiles();

	bool canAbort() const override;

  private slots:
	void startMoreParts();

  public slots:
	virtual void executeTask() override;
	virtual bool abort() override;

  private slots:
	void partProgress(int index, qint64 bytesReceived, qint64 bytesTotal);
	void partSucceeded(int index);
	void partFailed(int index);
	void partAborted(int index);

  private:
	shared_qobject_ptr<QNetworkAccessManager> m_network;

	struct part_info {
		qint64 current_progress = 0;
		qint64 total_progress = 1;
		int failures = 0;
	};
	QList<NetAction::Ptr> downloads;
	QList<part_info> parts_progress;
	QQueue<int> m_todo;
	QSet<int> m_doing;
	QSet<int> m_done;
	QSet<int> m_failed;
	qint64 m_current_progress = 0;
	bool m_aborted = false;
};
