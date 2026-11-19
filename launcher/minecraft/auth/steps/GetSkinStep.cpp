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

#include "GetSkinStep.h"

#include <QNetworkRequest>

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

GetSkinStep::GetSkinStep(AccountData* data) : AuthStep(data) {}

GetSkinStep::~GetSkinStep() noexcept = default;

QString GetSkinStep::describe()
{
	return tr("Getting skin.");
}

void GetSkinStep::perform()
{
	auto url = QUrl(m_data->minecraftProfile.skin.url);
	QNetworkRequest request = QNetworkRequest(url);
	AuthRequest* requestor = new AuthRequest(this);
	connect(requestor, &AuthRequest::finished, this,
			&GetSkinStep::onRequestDone);
	requestor->get(request);
}

void GetSkinStep::rehydrate()
{
	// NOOP, for now.
	// TODO: Make the most of this space.
}

void GetSkinStep::onRequestDone(QNetworkReply::NetworkError error,
								QByteArray data,
								QList<QNetworkReply::RawHeaderPair>)
{
	auto requestor = qobject_cast<AuthRequest*>(QObject::sender());
	requestor->deleteLater();

	if (error == QNetworkReply::NoError) {
		m_data->minecraftProfile.skin.data = data;
	}
	emit finished(AccountTaskState::STATE_SUCCEEDED, tr("Got skin"));
}
