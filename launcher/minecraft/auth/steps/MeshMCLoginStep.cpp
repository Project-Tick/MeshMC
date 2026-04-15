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

#include "MeshMCLoginStep.h"

#include <QNetworkRequest>

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"
#include "minecraft/auth/AccountTask.h"

MeshMCLoginStep::MeshMCLoginStep(AccountData* data) : AuthStep(data) {}

MeshMCLoginStep::~MeshMCLoginStep() noexcept = default;

QString MeshMCLoginStep::describe()
{
	return tr("Accessing Mojang services.");
}

void MeshMCLoginStep::perform()
{
	auto requestURL = "https://api.minecraftservices.com/launcher/login";
	auto uhs = m_data->mojangservicesToken.extra["uhs"].toString();
	auto xToken = m_data->mojangservicesToken.token;

	QString mc_auth_template = R"XXX(
{
    "xtoken": "XBL3.0 x=%1;%2",
    "platform": "PC_LAUNCHER"
}
)XXX";
	auto requestBody = mc_auth_template.arg(uhs, xToken);

	QNetworkRequest request = QNetworkRequest(QUrl(requestURL));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setRawHeader("Accept", "application/json");
	AuthRequest* requestor = new AuthRequest(this);
	connect(requestor, &AuthRequest::finished, this,
			&MeshMCLoginStep::onRequestDone);
	requestor->post(request, requestBody.toUtf8());
	qDebug() << "Getting Minecraft access token...";
}

void MeshMCLoginStep::rehydrate()
{
	// TODO: check the token validity
}

void MeshMCLoginStep::onRequestDone(QNetworkReply::NetworkError error,
									QByteArray data,
									QList<QNetworkReply::RawHeaderPair>)
{
	auto requestor = qobject_cast<AuthRequest*>(QObject::sender());
	requestor->deleteLater();

	qDebug() << data;
	if (error != QNetworkReply::NoError) {
		qWarning() << "Reply error:" << error;
#ifndef NDEBUG
		qDebug() << data;
#endif
		emit finished(AccountTaskState::STATE_FAILED_SOFT,
					  tr("Failed to get Minecraft access token: %1")
						  .arg(requestor->errorString_));
		return;
	}

	if (!Parsers::parseMojangResponse(data, m_data->yggdrasilToken)) {
		qWarning() << "Could not parse login_with_xbox response...";
#ifndef NDEBUG
		qDebug() << data;
#endif
		emit finished(
			AccountTaskState::STATE_FAILED_SOFT,
			tr("Failed to parse the Minecraft access token response."));
		return;
	}
	emit finished(AccountTaskState::STATE_WORKING, tr(""));
}
