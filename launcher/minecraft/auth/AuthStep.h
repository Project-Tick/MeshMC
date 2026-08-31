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
#include <QObject>
#include <QList>
#include <QNetworkReply>

#include "QObjectPtr.h"
#include "minecraft/auth/AccountData.h"
#include "AccountTask.h"

class AuthStep : public QObject
{
	Q_OBJECT

  public:
	using Ptr = shared_qobject_ptr<AuthStep>;

  public:
	explicit AuthStep(AccountData* data);
	virtual ~AuthStep() noexcept;

	virtual QString describe() = 0;

  public slots:
	virtual void perform() = 0;
	virtual void rehydrate() = 0;

  signals:
	void finished(AccountTaskState resultingState, QString message);
	void authorizeWithBrowser(const QUrl& url);

  protected:
	AccountData* m_data;
};
