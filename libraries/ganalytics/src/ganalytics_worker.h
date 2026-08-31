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

#include <QJsonObject>
#include <QDateTime>
#include <QTimer>
#include <QNetworkRequest>
#include <QQueue>

struct QueryBuffer {
	QJsonObject payload;
	QDateTime time;
};

class GAnalyticsWorker : public QObject
{
	Q_OBJECT

  public:
	explicit GAnalyticsWorker(GAnalytics* parent = 0);

	GAnalytics* q;

	QNetworkAccessManager* networkManager = nullptr;

	QQueue<QueryBuffer> m_messageQueue;
	QTimer m_timer;
	QNetworkRequest m_request;
	GAnalytics::LogLevel m_logLevel;

	QString m_trackingID;
	QString m_clientID;
	QString m_userID;
	QString m_appName;
	QString m_appVersion;
	QString m_language;
	QString m_screenResolution;
	QString m_viewportSize;

	QString m_measurementId;
	QString m_apiSecret;
	QString m_sessionId;

	bool m_debugMode = false;
	bool m_anonymizeIPs = false;
	bool m_isEnabled = false;
	int m_timerInterval = 30000;
	int m_version = 0;

	const static int fourHours = 4 * 60 * 60 * 1000;
	const static QLatin1String dateTimeFormat;

  public:
	void logMessage(GAnalytics::LogLevel level, const QString& message);

	QJsonObject buildBasePayload();
	QUrl buildRequestUrl();
	QString getScreenResolution();
	QString getUserAgent();
	QList<QString> persistMessageQueue();
	void readMessagesFromFile(const QList<QString>& dataList);

	void enqueuePayload(const QJsonObject& payload);
	void setIsSending(bool doSend);
	void enable(bool state);

  public slots:
	void postMessage();
	void postMessageFinished();
};
