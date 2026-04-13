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

#include "CapeChange.h"

#include <QNetworkRequest>
#include <QHttpMultiPart>

#include "Application.h"

CapeChange::CapeChange(QObject* parent, QString token, QString cape)
	: Task(parent), m_capeId(cape), m_token(token)
{
}

void CapeChange::setCape(QString& cape)
{
	QNetworkRequest request(QUrl(
		"https://api.minecraftservices.com/minecraft/profile/capes/active"));
	auto requestString = QString("{\"capeId\":\"%1\"}").arg(m_capeId);
	request.setRawHeader("Authorization",
						 QString("Bearer %1").arg(m_token).toLocal8Bit());
	QNetworkReply* rep =
		APPLICATION->network()->put(request, requestString.toUtf8());

	setStatus(tr("Equipping cape"));

	m_reply = shared_qobject_ptr<QNetworkReply>(rep);
	connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
	connect(rep, &QNetworkReply::errorOccurred, this,
			&CapeChange::downloadError);
	connect(rep, &QNetworkReply::finished, this, &CapeChange::downloadFinished);
}

void CapeChange::clearCape()
{
	QNetworkRequest request(QUrl(
		"https://api.minecraftservices.com/minecraft/profile/capes/active"));
	auto requestString = QString("{\"capeId\":\"%1\"}").arg(m_capeId);
	request.setRawHeader("Authorization",
						 QString("Bearer %1").arg(m_token).toLocal8Bit());
	QNetworkReply* rep = APPLICATION->network()->deleteResource(request);

	setStatus(tr("Removing cape"));

	m_reply = shared_qobject_ptr<QNetworkReply>(rep);
	connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
	connect(rep, &QNetworkReply::errorOccurred, this,
			&CapeChange::downloadError);
	connect(rep, &QNetworkReply::finished, this, &CapeChange::downloadFinished);
}

void CapeChange::executeTask()
{
	if (m_capeId.isEmpty()) {
		clearCape();
	} else {
		setCape(m_capeId);
	}
}

void CapeChange::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	qCritical() << "Network error: " << error;
	emitFailed(m_reply->errorString());
}

void CapeChange::downloadFinished()
{
	// if the download failed
	if (m_reply->error() != QNetworkReply::NetworkError::NoError) {
		emitFailed(QString("Network error: %1").arg(m_reply->errorString()));
		m_reply.reset();
		return;
	}
	emitSucceeded();
}
