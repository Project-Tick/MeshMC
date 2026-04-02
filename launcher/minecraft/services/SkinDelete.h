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

#include <QFile>
#include <QtNetwork/QtNetwork>
#include "tasks/Task.h"

typedef shared_qobject_ptr<class SkinDelete> SkinDeletePtr;

class SkinDelete : public Task
{
	Q_OBJECT
  public:
	SkinDelete(QObject* parent, QString token);
	virtual ~SkinDelete() = default;

  private:
	QString m_token;
	shared_qobject_ptr<QNetworkReply> m_reply;

  protected:
	virtual void executeTask();

  public slots:
	void downloadError(QNetworkReply::NetworkError);
	void downloadFinished();
};
