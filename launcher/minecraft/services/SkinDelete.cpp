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

#include "SkinDelete.h"

#include <QNetworkRequest>
#include <QHttpMultiPart>

#include "Application.h"

SkinDelete::SkinDelete(QObject* parent, QString token)
	: Task(parent), m_token(token)
{
}

void SkinDelete::executeTask()
{
	QNetworkRequest request(QUrl(
		"https://api.minecraftservices.com/minecraft/profile/skins/active"));
	request.setRawHeader("Authorization",
						 QString("Bearer %1").arg(m_token).toLocal8Bit());
	QNetworkReply* rep = APPLICATION->network()->deleteResource(request);
	m_reply = shared_qobject_ptr<QNetworkReply>(rep);

	setStatus(tr("Deleting skin"));
	connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
	connect(rep, &QNetworkReply::errorOccurred, this,
			&SkinDelete::downloadError);
	connect(rep, &QNetworkReply::finished, this, &SkinDelete::downloadFinished);
}

void SkinDelete::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	qCritical() << "Network error: " << error;
	emitFailed(m_reply->errorString());
}

void SkinDelete::downloadFinished()
{
	// if the download failed
	if (m_reply->error() != QNetworkReply::NetworkError::NoError) {
		emitFailed(QString("Network error: %1").arg(m_reply->errorString()));
		m_reply.reset();
		return;
	}
	emitSucceeded();
}
