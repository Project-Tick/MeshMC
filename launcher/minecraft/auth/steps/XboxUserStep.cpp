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
