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

#include <QObject>
#include <QVariantMap>

class QNetworkAccessManager;
class GAnalyticsWorker;

class GAnalytics : public QObject
{
	Q_OBJECT
	Q_ENUMS(LogLevel)

  public:
	explicit GAnalytics(const QString& trackingID, const QString& clientID,
						const int version, QObject* parent = 0);
	~GAnalytics();

  public:
	enum LogLevel { Debug, Info, Error };

	int version();

	void setLogLevel(LogLevel logLevel);
	LogLevel logLevel() const;

	// Getter and Setters
	void setViewportSize(const QString& viewportSize);
	QString viewportSize() const;

	void setLanguage(const QString& language);
	QString language() const;

	void setAnonymizeIPs(bool anonymize);
	bool anonymizeIPs();

	void setSendInterval(int milliseconds);
	int sendInterval() const;

	void enable(bool state = true);
	bool isEnabled();

	/// Get or set the network access manager. If none is set, the class creates
	/// its own on the first request
	void setNetworkAccessManager(QNetworkAccessManager* networkAccessManager);
	QNetworkAccessManager* networkAccessManager() const;

	void setMeasurementId(const QString& measurementId);
	QString measurementId() const;

	void setApiSecret(const QString& apiSecret);
	QString apiSecret() const;

	void setDebugMode(bool debugMode);
	bool debugMode() const;

	void setSessionId(const QString& sessionId);
	QString sessionId() const;

  public slots:
	void sendScreenView(const QString& screenName,
						const QVariantMap& customValues = QVariantMap());
	void sendEvent(const QString& category, const QString& action,
				   const QString& label = QString(),
				   const QVariant& value = QVariant(),
				   const QVariantMap& customValues = QVariantMap());
	void sendException(const QString& exceptionDescription,
					   bool exceptionFatal = true,
					   const QVariantMap& customValues = QVariantMap());
	void startSession();
	void endSession();

  private:
	GAnalyticsWorker* d;

	friend QDataStream& operator<<(QDataStream& outStream,
								   const GAnalytics& analytics);
	friend QDataStream& operator>>(QDataStream& inStream,
								   GAnalytics& analytics);
};

QDataStream& operator<<(QDataStream& outStream, const GAnalytics& analytics);
QDataStream& operator>>(QDataStream& inStream, GAnalytics& analytics);
