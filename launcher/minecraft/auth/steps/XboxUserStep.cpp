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

#include "XboxUserStep.h"

#include <QNetworkRequest>

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

XboxUserStep::XboxUserStep(AccountData* data) : AuthStep(data) {}

XboxUserStep::~XboxUserStep() noexcept = default;

QString XboxUserStep::describe()
{
	return tr("Logging in as an Xbox user.");
}

void XboxUserStep::rehydrate()
{
	// NOOP, for now. We only save bools and there's nothing to check.
}

void XboxUserStep::perform()
{
	QString xbox_auth_template = R"XXX(
{
    "Properties": {
        "AuthMethod": "RPS",
        "SiteName": "user.auth.xboxlive.com",
        "RpsTicket": "d=%1"
    },
    "RelyingParty": "http://auth.xboxlive.com",
    "TokenType": "JWT"
}
)XXX";
	auto xbox_auth_data = xbox_auth_template.arg(m_data->msaToken.token);

	QNetworkRequest request = QNetworkRequest(
		QUrl("https://user.auth.xboxlive.com/user/authenticate"));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setRawHeader("Accept", "application/json");
	auto* requestor = new AuthRequest(this);
	connect(requestor, &AuthRequest::finished, this,
			&XboxUserStep::onRequestDone);
	requestor->post(request, xbox_auth_data.toUtf8());
	qDebug() << "First layer of XBox auth ... commencing.";
}

void XboxUserStep::onRequestDone(QNetworkReply::NetworkError error,
								 QByteArray data,
								 QList<QNetworkReply::RawHeaderPair>)
{
	auto requestor = qobject_cast<AuthRequest*>(QObject::sender());
	requestor->deleteLater();

	if (error != QNetworkReply::NoError) {
		qWarning() << "Reply error:" << error;
		emit finished(AccountTaskState::STATE_FAILED_SOFT,
					  tr("XBox user authentication failed."));
		return;
	}

	Katabasis::Token temp;
	if (!Parsers::parseXTokenResponse(data, temp, "UToken")) {
		qWarning() << "Could not parse user authentication response...";
		emit finished(
			AccountTaskState::STATE_FAILED_SOFT,
			tr("XBox user authentication response could not be understood."));
		return;
	}
	m_data->userToken = temp;
	emit finished(AccountTaskState::STATE_WORKING, tr("Got Xbox user token"));
}
