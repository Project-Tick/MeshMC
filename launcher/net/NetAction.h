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

#include <QObject>
#include <QUrl>
#include <memory>
#include <QNetworkReply>
#include <QObjectPtr.h>

enum JobStatus {
	Job_NotStarted,
	Job_InProgress,
	Job_Finished,
	Job_Failed,
	Job_Aborted,
	/*
	 * FIXME: @NUKE this confuses the task failing with us having a fallback in
	 * the form of local data. Clear up the confusion. Same could be true for
	 * aborted task - the presence of pre-existing result is a separate concern
	 */
	Job_Failed_Proceed
};

class NetAction : public QObject
{
	Q_OBJECT
  protected:
	explicit NetAction() : QObject(nullptr) {};

  public:
	using Ptr = shared_qobject_ptr<NetAction>;

	virtual ~NetAction() {};

	bool isRunning() const
	{
		return m_status == Job_InProgress;
	}
	bool isFinished() const
	{
		return m_status >= Job_Finished;
	}
	bool wasSuccessful() const
	{
		return m_status == Job_Finished || m_status == Job_Failed_Proceed;
	}

	qint64 totalProgress() const
	{
		return m_total_progress;
	}
	qint64 currentProgress() const
	{
		return m_progress;
	}
	virtual bool abort()
	{
		return false;
	}
	virtual bool canAbort()
	{
		return false;
	}
	QUrl url()
	{
		return m_url;
	}

  signals:
	void started(int index);
	void netActionProgress(int index, qint64 current, qint64 total);
	void succeeded(int index);
	void failed(int index);
	void aborted(int index);

  protected slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) = 0;
	virtual void downloadError(QNetworkReply::NetworkError error) = 0;
	virtual void downloadFinished() = 0;
	virtual void downloadReadyRead() = 0;

  public slots:
	void start(shared_qobject_ptr<QNetworkAccessManager> network)
	{
		m_network = network;
		startImpl();
	}

  protected:
	virtual void startImpl() = 0;

  public:
	shared_qobject_ptr<QNetworkAccessManager> m_network;

	/// index within the parent job, FIXME: nuke
	int m_index_within_job = 0;

	/// the network reply
	unique_qobject_ptr<QNetworkReply> m_reply;

	/// source URL
	QUrl m_url;

	qint64 m_progress = 0;
	qint64 m_total_progress = 1;

  protected:
	JobStatus m_status = Job_NotStarted;
};
