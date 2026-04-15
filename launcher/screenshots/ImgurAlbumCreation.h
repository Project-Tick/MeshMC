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
#include "net/NetAction.h"
#include "Screenshot.h"
#include "QObjectPtr.h"

typedef shared_qobject_ptr<class ImgurAlbumCreation> ImgurAlbumCreationPtr;
class ImgurAlbumCreation : public NetAction
{
  public:
	explicit ImgurAlbumCreation(QList<ScreenShot::Ptr> screenshots);
	static ImgurAlbumCreationPtr make(QList<ScreenShot::Ptr> screenshots)
	{
		return ImgurAlbumCreationPtr(new ImgurAlbumCreation(screenshots));
	}

	QString deleteHash() const
	{
		return m_deleteHash;
	}
	QString id() const
	{
		return m_id;
	}

  protected slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	virtual void downloadError(QNetworkReply::NetworkError error);
	virtual void downloadFinished();
	virtual void downloadReadyRead() {}

  public slots:
	virtual void startImpl();

  private:
	QList<ScreenShot::Ptr> m_screenshots;

	QString m_deleteHash;
	QString m_id;
};
