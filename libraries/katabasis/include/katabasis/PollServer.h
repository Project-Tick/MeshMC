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

#include <QByteArray>
#include <QMap>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QTimer>

class QNetworkAccessManager;

namespace Katabasis
{

	/// Poll an authorization server for token
	class PollServer : public QObject
	{
		Q_OBJECT

	  public:
		explicit PollServer(QNetworkAccessManager* manager,
							const QNetworkRequest& request,
							const QByteArray& payload, int expiresIn,
							QObject* parent = 0);

		/// Seconds to wait between polling requests
		Q_PROPERTY(int interval READ interval WRITE setInterval)
		int interval() const;
		void setInterval(int interval);

	  signals:
		void verificationReceived(QMap<QString, QString>);
		void serverClosed(bool); // whether it has found parameters

	  public slots:
		void startPolling();

	  protected slots:
		void onPollTimeout();
		void onExpiration();
		void onReplyFinished();

	  protected:
		QNetworkAccessManager* manager_;
		const QNetworkRequest request_;
		const QByteArray payload_;
		const int expiresIn_;
		QTimer expirationTimer;
		QTimer pollTimer;
	};

} // namespace Katabasis
