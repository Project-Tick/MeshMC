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

#include "SkinUpload.h"

#include <QNetworkRequest>
#include <QHttpMultiPart>

#include "Application.h"

QByteArray getVariant(SkinUpload::Model model)
{
	switch (model) {
		default:
			qDebug() << "Unknown skin type!";
		case SkinUpload::STEVE:
			return "CLASSIC";
		case SkinUpload::ALEX:
			return "SLIM";
	}
}

SkinUpload::SkinUpload(QObject* parent, QString token, QByteArray skin,
					   SkinUpload::Model model)
	: Task(parent), m_model(model), m_skin(skin), m_token(token)
{
}

void SkinUpload::executeTask()
{
	QNetworkRequest request(
		QUrl("https://api.minecraftservices.com/minecraft/profile/skins"));
	request.setRawHeader("Authorization",
						 QString("Bearer %1").arg(m_token).toLocal8Bit());
	QHttpMultiPart* multiPart =
		new QHttpMultiPart(QHttpMultiPart::FormDataType);

	QHttpPart skin;
	skin.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
	skin.setHeader(QNetworkRequest::ContentDispositionHeader,
				   QVariant("form-data; name=\"file\"; filename=\"skin.png\""));
	skin.setBody(m_skin);

	QHttpPart model;
	model.setHeader(QNetworkRequest::ContentDispositionHeader,
					QVariant("form-data; name=\"variant\""));
	model.setBody(getVariant(m_model));

	multiPart->append(skin);
	multiPart->append(model);

	QNetworkReply* rep = APPLICATION->network()->post(request, multiPart);
	m_reply = shared_qobject_ptr<QNetworkReply>(rep);

	setStatus(tr("Uploading skin"));
	connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
	connect(rep, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), this,
			SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(rep, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void SkinUpload::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	qCritical() << "Network error: " << error;
	emitFailed(m_reply->errorString());
}

void SkinUpload::downloadFinished()
{
	// if the download failed
	if (m_reply->error() != QNetworkReply::NetworkError::NoError) {
		emitFailed(QString("Network error: %1").arg(m_reply->errorString()));
		m_reply.reset();
		return;
	}
	emitSucceeded();
}
