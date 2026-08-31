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

#include <QFile>
#include <QtNetwork/QtNetwork>
#include <memory>
#include "tasks/Task.h"

typedef shared_qobject_ptr<class SkinUpload> SkinUploadPtr;

class SkinUpload : public Task
{
	Q_OBJECT
  public:
	enum Model { STEVE, ALEX };

	// Note this class takes ownership of the file.
	SkinUpload(QObject* parent, QString token, QByteArray skin,
			   Model model = STEVE);
	virtual ~SkinUpload() {}

  private:
	Model m_model;
	QByteArray m_skin;
	QString m_token;
	shared_qobject_ptr<QNetworkReply> m_reply;

  protected:
	virtual void executeTask();

  public slots:

	void downloadError(QNetworkReply::NetworkError);

	void downloadFinished();
};
