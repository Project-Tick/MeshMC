/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Project Tick
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
