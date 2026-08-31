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
#include "QObjectPtr.h"
#include "net/NetAction.h"
#include "Screenshot.h"

class ImgurUpload : public NetAction
{
  public:
	using Ptr = shared_qobject_ptr<ImgurUpload>;

	explicit ImgurUpload(ScreenShot::Ptr shot);
	static Ptr make(ScreenShot::Ptr shot)
	{
		return Ptr(new ImgurUpload(shot));
	}

  protected slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) override;
	void downloadError(QNetworkReply::NetworkError error) override;
	void downloadFinished() override;
	void downloadReadyRead() override {}

  public slots:
	void startImpl() override;

  private:
	ScreenShot::Ptr m_shot;
	bool finished = true;
};
