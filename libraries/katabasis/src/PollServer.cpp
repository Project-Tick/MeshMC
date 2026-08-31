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

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "katabasis/PollServer.h"
#include "JsonResponse.h"

namespace
{
	QMap<QString, QString> toVerificationParams(const QVariantMap& map)
	{
		QMap<QString, QString> params;
		for (QVariantMap::const_iterator i = map.constBegin();
			 i != map.constEnd(); ++i) {
			params[i.key()] = i.value().toString();
		}
		return params;
	}
} // namespace

namespace Katabasis
{

	PollServer::PollServer(QNetworkAccessManager* manager,
						   const QNetworkRequest& request,
						   const QByteArray& payload, int expiresIn,
						   QObject* parent)
		: QObject(parent), manager_(manager), request_(request),
		  payload_(payload), expiresIn_(expiresIn)
	{
		expirationTimer.setTimerType(Qt::VeryCoarseTimer);
		expirationTimer.setInterval(expiresIn * 1000);
		expirationTimer.setSingleShot(true);
		connect(&expirationTimer, &QTimer::timeout, this,
				&PollServer::onExpiration);
		expirationTimer.start();

		pollTimer.setTimerType(Qt::VeryCoarseTimer);
		pollTimer.setInterval(5 * 1000);
		pollTimer.setSingleShot(true);
		connect(&pollTimer, &QTimer::timeout, this, &PollServer::onPollTimeout);
	}

	int PollServer::interval() const
	{
		return pollTimer.interval() / 1000;
	}

	void PollServer::setInterval(int interval)
	{
		pollTimer.setInterval(interval * 1000);
	}

	void PollServer::startPolling()
	{
		if (expirationTimer.isActive()) {
			pollTimer.start();
		}
	}

	void PollServer::onPollTimeout()
	{
		qDebug() << "PollServer::onPollTimeout: retrying";
		QNetworkReply* reply = manager_->post(request_, payload_);
		connect(reply, &QNetworkReply::finished, this,
				&PollServer::onReplyFinished);
	}

	void PollServer::onExpiration()
	{
		pollTimer.stop();
		emit serverClosed(false);
	}

	void PollServer::onReplyFinished()
	{
		QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

		if (!reply) {
			qDebug() << "PollServer::onReplyFinished: reply is null";
			return;
		}

		QByteArray replyData = reply->readAll();
		QMap<QString, QString> params =
			toVerificationParams(parseJsonResponse(replyData));

		// Dump replyData
		// SENSITIVE DATA in RelWithDebInfo or Debug builds
		// qDebug() << "PollServer::onReplyFinished: replyData\n";
		// qDebug() << QString( replyData );

		if (reply->error() == QNetworkReply::TimeoutError) {
			// rfc8628#section-3.2
			// "On encountering a connection timeout, clients MUST unilaterally
			// reduce their polling frequency before retrying.  The use of an
			// exponential backoff algorithm to achieve this, such as doubling
			// the polling interval on each such connection timeout, is
			// RECOMMENDED."
			setInterval(interval() * 2);
			pollTimer.start();
		} else {
			QString error = params.value("error");
			if (error == "slow_down") {
				// rfc8628#section-3.2
				// "A variant of 'authorization_pending', the authorization
				// request is still pending and polling should continue, but the
				// interval MUST be increased by 5 seconds for this and all
				// subsequent requests."
				setInterval(interval() + 5);
				pollTimer.start();
			} else if (error == "authorization_pending") {
				// keep trying - rfc8628#section-3.2
				// "The authorization request is still pending as the end user
				// hasn't yet completed the user-interaction steps
				// (Section 3.3)."
				pollTimer.start();
			} else {
				expirationTimer.stop();
				emit serverClosed(true);
				// let O2 handle the other cases
				emit verificationReceived(params);
			}
		}
		reply->deleteLater();
	}

} // namespace Katabasis
