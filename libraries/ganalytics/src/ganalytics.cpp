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

#include "ganalytics.h"
#include "ganalytics_worker.h"
#include "sys.h"

#include <QDataStream>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQueue>
#include <QSettings>
#include <QTimer>
#include <QUuid>

GAnalytics::GAnalytics(const QString& trackingID, const QString& clientID,
					   const int version, QObject* parent)
	: QObject(parent)
{
	d = new GAnalyticsWorker(this);
	d->m_trackingID = trackingID;
	d->m_clientID = clientID;
	d->m_version = version;
}

/**
 * Destructor of class GAnalytics.
 */
GAnalytics::~GAnalytics()
{
	delete d;
}

void GAnalytics::setLogLevel(GAnalytics::LogLevel logLevel)
{
	d->m_logLevel = logLevel;
}

GAnalytics::LogLevel GAnalytics::logLevel() const
{
	return d->m_logLevel;
}

// SETTER and GETTER
void GAnalytics::setViewportSize(const QString& viewportSize)
{
	d->m_viewportSize = viewportSize;
}

QString GAnalytics::viewportSize() const
{
	return d->m_viewportSize;
}

void GAnalytics::setLanguage(const QString& language)
{
	d->m_language = language;
}

QString GAnalytics::language() const
{
	return d->m_language;
}

void GAnalytics::setAnonymizeIPs(bool anonymize)
{
	d->m_anonymizeIPs = anonymize;
}

bool GAnalytics::anonymizeIPs()
{
	return d->m_anonymizeIPs;
}

void GAnalytics::setSendInterval(int milliseconds)
{
	d->m_timer.setInterval(milliseconds);
}

int GAnalytics::sendInterval() const
{
	return (d->m_timer.interval());
}

bool GAnalytics::isEnabled()
{
	return d->m_isEnabled;
}

void GAnalytics::enable(bool state)
{
	d->enable(state);
}

int GAnalytics::version()
{
	return d->m_version;
}

void GAnalytics::setMeasurementId(const QString& measurementId)
{
	d->m_measurementId = measurementId;
}

QString GAnalytics::measurementId() const
{
	return d->m_measurementId;
}

void GAnalytics::setApiSecret(const QString& apiSecret)
{
	d->m_apiSecret = apiSecret;
}

QString GAnalytics::apiSecret() const
{
	return d->m_apiSecret;
}

void GAnalytics::setDebugMode(bool debugMode)
{
	d->m_debugMode = debugMode;
}

bool GAnalytics::debugMode() const
{
	return d->m_debugMode;
}

void GAnalytics::setSessionId(const QString& sessionId)
{
	d->m_sessionId = sessionId;
}

QString GAnalytics::sessionId() const
{
	return d->m_sessionId;
}

void GAnalytics::setNetworkAccessManager(
	QNetworkAccessManager* networkAccessManager)
{
	if (d->networkManager != networkAccessManager) {
		// Delete the old network manager if it was our child
		if (d->networkManager && d->networkManager->parent() == this) {
			d->networkManager->deleteLater();
		}

		d->networkManager = networkAccessManager;
	}
}

QNetworkAccessManager* GAnalytics::networkAccessManager() const
{
	return d->networkManager;
}

static void appendCustomValues(QJsonObject& params,
							   const QVariantMap& customValues)
{
	for (QVariantMap::const_iterator iter = customValues.begin();
		 iter != customValues.end(); ++iter) {
		params[iter.key()] = QJsonValue::fromVariant(iter.value());
	}
}

void GAnalytics::sendScreenView(const QString& screenName,
								const QVariantMap& customValues)
{
	d->logMessage(Info, QString("ScreenView: %1").arg(screenName));

	QJsonObject payload = d->buildBasePayload();

	QJsonObject params;
	params["screen_name"] = screenName;
	params["app_name"] = d->m_appName;
	params["app_version"] = d->m_appVersion;
	params["language"] = d->m_language;
	params["screen_resolution"] = d->m_screenResolution;
	params["engagement_time_msec"] = QStringLiteral("100");
	if (!d->m_sessionId.isEmpty()) {
		params["session_id"] = d->m_sessionId;
	}
	appendCustomValues(params, customValues);

	QJsonObject event;
	event["name"] = QStringLiteral("screen_view");
	event["params"] = params;

	QJsonArray events;
	events.append(event);
	payload["events"] = events;

	d->enqueuePayload(payload);
}

/**
 * This method is called whenever a button was pressed in the application.
 * A query for a POST message will be created to report this event. The
 * created query will be stored in a message queue.
 */
void GAnalytics::sendEvent(const QString& category, const QString& action,
						   const QString& label, const QVariant& value,
						   const QVariantMap& customValues)
{
	QJsonObject payload = d->buildBasePayload();

	QJsonObject params;
	params["event_category"] = category;
	params["event_action"] = action;
	if (!label.isEmpty())
		params["event_label"] = label;
	if (value.isValid())
		params["value"] = QJsonValue::fromVariant(value);
	params["app_name"] = d->m_appName;
	params["app_version"] = d->m_appVersion;
	params["language"] = d->m_language;
	params["screen_resolution"] = d->m_screenResolution;
	params["engagement_time_msec"] = QStringLiteral("100");
	if (!d->m_sessionId.isEmpty()) {
		params["session_id"] = d->m_sessionId;
	}
	appendCustomValues(params, customValues);

	QString eventName = category.toLower().replace(" ", "_");
	QJsonObject event;
	event["name"] = eventName;
	event["params"] = params;

	QJsonArray events;
	events.append(event);
	payload["events"] = events;

	d->enqueuePayload(payload);
}

/**
 * Method is called after an exception was raised. It builds a
 * query for a POST message. These query will be stored in a
 * message queue.
 */
void GAnalytics::sendException(const QString& exceptionDescription,
							   bool exceptionFatal,
							   const QVariantMap& customValues)
{
	QJsonObject payload = d->buildBasePayload();

	QJsonObject params;
	params["description"] = exceptionDescription;
	params["fatal"] = exceptionFatal;
	params["app_name"] = d->m_appName;
	params["app_version"] = d->m_appVersion;
	params["language"] = d->m_language;
	params["engagement_time_msec"] = QStringLiteral("100");
	if (!d->m_sessionId.isEmpty()) {
		params["session_id"] = d->m_sessionId;
	}
	appendCustomValues(params, customValues);

	QJsonObject event;
	event["name"] = QStringLiteral("app_exception");
	event["params"] = params;

	QJsonArray events;
	events.append(event);
	payload["events"] = events;

	d->enqueuePayload(payload);
}

/**
 * Session starts. This event will be sent by a POST message.
 * Query is setup in this method and stored in the message
 * queue.
 */
void GAnalytics::startSession()
{
	QJsonObject payload = d->buildBasePayload();

	QJsonObject params;
	params["app_name"] = d->m_appName;
	params["app_version"] = d->m_appVersion;
	params["engagement_time_msec"] = QStringLiteral("100");
	if (!d->m_sessionId.isEmpty()) {
		params["session_id"] = d->m_sessionId;
	}

	QJsonObject event;
	event["name"] = QStringLiteral("session_start");
	event["params"] = params;

	QJsonArray events;
	events.append(event);
	payload["events"] = events;

	d->enqueuePayload(payload);
}

void GAnalytics::endSession()
{
	QJsonObject payload = d->buildBasePayload();

	QJsonObject params;
	params["app_name"] = d->m_appName;
	params["app_version"] = d->m_appVersion;
	params["engagement_time_msec"] = QStringLiteral("100");
	if (!d->m_sessionId.isEmpty()) {
		params["session_id"] = d->m_sessionId;
	}

	QJsonObject event;
	event["name"] = QStringLiteral("session_end");
	event["params"] = params;

	QJsonArray events;
	events.append(event);
	payload["events"] = events;

	d->enqueuePayload(payload);
}

/**
 * Qut stream to persist class GAnalytics.
 */
QDataStream& operator<<(QDataStream& outStream, const GAnalytics& analytics)
{
	outStream << analytics.d->persistMessageQueue();

	return outStream;
}

/**
 * In stream to read GAnalytics from file.
 */
QDataStream& operator>>(QDataStream& inStream, GAnalytics& analytics)
{
	QList<QString> dataList;
	inStream >> dataList;
	analytics.d->readMessagesFromFile(dataList);

	return inStream;
}
