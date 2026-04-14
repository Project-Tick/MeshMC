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

#include "XboxProfileStep.h"

#include <QNetworkRequest>
#include <QUrlQuery>

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

XboxProfileStep::XboxProfileStep(AccountData* data) : AuthStep(data) {}

XboxProfileStep::~XboxProfileStep() noexcept = default;

QString XboxProfileStep::describe()
{
	return tr("Fetching Xbox profile.");
}

void XboxProfileStep::rehydrate()
{
	// NOOP, for now. We only save bools and there's nothing to check.
}

void XboxProfileStep::perform()
{
	auto url = QUrl("https://profile.xboxlive.com/users/me/profile/settings");
	QUrlQuery q;
	q.addQueryItem(
		"settings",
		"GameDisplayName,AppDisplayName,AppDisplayPicRaw,GameDisplayPicRaw,"
		"PublicGamerpic,ShowUserAsAvatar,Gamerscore,Gamertag,ModernGamertag,"
		"ModernGamertagSuffix,"
		"UniqueModernGamertag,AccountTier,TenureLevel,XboxOneRep,"
		"PreferredColor,Location,Bio,Watermarks,"
		"RealName,RealNameOverride,IsQuarantined");
	url.setQuery(q);

	QNetworkRequest request = QNetworkRequest(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setRawHeader("Accept", "application/json");
	request.setRawHeader("x-xbl-contract-version", "3");
	request.setRawHeader("Authorization",
						 QString("XBL3.0 x=%1;%2")
							 .arg(m_data->userToken.extra["uhs"].toString(),
								  m_data->xboxApiToken.token)
							 .toUtf8());
	AuthRequest* requestor = new AuthRequest(this);
	connect(requestor, &AuthRequest::finished, this,
			&XboxProfileStep::onRequestDone);
	requestor->get(request);
	qDebug() << "Getting Xbox profile...";
}

void XboxProfileStep::onRequestDone(QNetworkReply::NetworkError error,
									QByteArray data,
									QList<QNetworkReply::RawHeaderPair>)
{
	auto requestor = qobject_cast<AuthRequest*>(QObject::sender());
	requestor->deleteLater();

	if (error != QNetworkReply::NoError) {
		qWarning() << "Reply error:" << error;
#ifndef NDEBUG
		qDebug() << data;
#endif
		finished(AccountTaskState::STATE_FAILED_SOFT,
				 tr("Failed to retrieve the Xbox profile."));
		return;
	}

#ifndef NDEBUG
	qDebug() << "XBox profile: " << data;
#endif

	emit finished(AccountTaskState::STATE_WORKING, tr("Got Xbox profile"));
}
