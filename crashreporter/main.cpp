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

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QMap>
#include <QString>

#include "CrashReportDialog.h"

static QString readLogFile(const QString& logDir, const QString& baseName)
{
	QString logPath =
		QDir(logDir).absoluteFilePath(QString("%1-0.log").arg(baseName));
	QFile file(logPath);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		qWarning() << "Could not open log file:" << logPath;
		return QString();
	}
	return QString::fromUtf8(file.readAll());
}

static QString censorText(QString text, const QMap<QString, QString>& filter)
{
	QStringList keys = filter.keys();
	std::sort(keys.begin(), keys.end(),
			  [](const QString& s1, const QString& s2) {
				  return s1.length() > s2.length();
			  });

	for (const QString& key : keys) {
		if (!key.isEmpty()) {
			text.replace(key, filter[key]);
		}
	}
	return text;
}

static QString uploadToPasteEE(QNetworkAccessManager* nam, const QString& text,
							   const QString& apiKey)
{
	QJsonObject sectionObject;
	sectionObject.insert("name", "Crash Log");
	sectionObject.insert("syntax", "text");
	sectionObject.insert("contents", text);

	QJsonArray sectionArray;
	sectionArray.append(sectionObject);

	QJsonObject topLevelObj;
	topLevelObj.insert("description", "MeshMC Crash Log");
	topLevelObj.insert("sections", sectionArray);

	QJsonDocument docOut;
	docOut.setObject(topLevelObj);

	QByteArray jsonContent = docOut.toJson(QJsonDocument::Compact);

	// Validate size (2MB for public, 12MB for keyed)
	int maxSize = (apiKey == "public") ? (1024 * 1024 * 2) : (1024 * 1024 * 12);
	if (jsonContent.size() > maxSize) {
		qWarning() << "Log file too large for paste.ee upload";
		return QString();
	}

	QNetworkRequest request(QUrl("https://api.paste.ee/v1/pastes"));
	request.setHeader(QNetworkRequest::UserAgentHeader,
					  "MeshMC-CrashReporter/1.0");
	request.setHeader(QNetworkRequest::ContentTypeHeader,
					  "application/json; charset=utf-8");
	if (apiKey != "public" && !apiKey.isEmpty()) {
		request.setRawHeader("X-Auth-Token", apiKey.toUtf8());
	}

	QNetworkReply* reply = nam->post(request, jsonContent);

	// Synchronous wait
	QEventLoop loop;
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	// Timeout after 30 seconds
	QTimer timer;
	timer.setSingleShot(true);
	QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	timer.start(30000);
	loop.exec();

	if (reply->error() != QNetworkReply::NoError) {
		QByteArray errorBody = reply->readAll();
		qWarning() << "Network error during paste.ee upload:"
				   << reply->errorString();
		qWarning() << "Paste.ee detailed error response:" << errorBody;
		reply->deleteLater();
		return QString();
	}

	QByteArray data = reply->readAll();
	reply->deleteLater();

	QJsonParseError jsonError;
	QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
	if (jsonError.error != QJsonParseError::NoError) {
		qWarning() << "JSON parse error:" << jsonError.errorString();
		return QString();
	}

	auto object = doc.object();
	if (!object.value("success").toBool()) {
		qWarning() << "paste.ee reported error:"
				   << object.value("error").toString();
		return QString();
	}

	return object.value("link").toString();
}

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	app.setApplicationName("meshmc-crashreporter");
	app.setApplicationVersion("1.0");

	QCommandLineParser parser;
	parser.setApplicationDescription("MeshMC Crash Reporter");
	parser.addHelpOption();

	QCommandLineOption logDirOption(
		"logdir", "Directory containing MeshMC log files.", "directory");
	parser.addOption(logDirOption);

	QCommandLineOption baseNameOption(
		"name", "Base name for log files (default: MeshMC).", "name", "MeshMC");
	parser.addOption(baseNameOption);

	QCommandLineOption apiKeyOption(
		"apikey", "paste.ee API key (default: public).", "key", "public");
	parser.addOption(apiKeyOption);

	parser.process(app);

	QString logDir = parser.value(logDirOption);
	if (logDir.isEmpty()) {
		logDir = QDir::currentPath();
	}

	QString baseName = parser.value(baseNameOption);
	QString apiKey = parser.value(apiKeyOption);

	// Read the crash log
	QString logContent = readLogFile(logDir, baseName);
	if (logContent.isEmpty()) {
		QMessageBox::warning(nullptr, "MeshMC Crash Reporter",
							 "Could not read MeshMC log file.\n"
							 "The crash reporter cannot proceed.");
		return 1;
	}

	QMap<QString, QString> filter;

	QString homePath = QDir::homePath();
	if (!homePath.isEmpty()) {
		filter[homePath] = "/home/<USERNAME>";
	}

	QString userName = qEnvironmentVariable("USER");
	if (userName.isEmpty()) {
		userName = qEnvironmentVariable("USERNAME");
	}

	if (!userName.isEmpty()) {
		filter["/" + userName + "/"] = "/<USERNAME>";
	}

	QRegularExpression authLineRegex(
		R"((?im)^.*(?:access_token|refresh_token|id_token|token|authorization|utoken|xsts|xbl|IssueInstant|NotAfter|DisplayClaims|xuid|uhs|xid|gtg|usr|utr|prv|ugc).*$)");

	QRegularExpression bearerHeader(
		R"((?i)\bBearer\s+[A-Za-z0-9._~+/=-]{20,}\b)");

	QRegularExpression jwtFirstRegex(
		R"(\b[A-Za-z0-9_-]{10,}(?:\.[A-Za-z0-9_-]{10,}){2,4}\b)");

	QRegularExpression jwtSecondRegex(
		R"(\beyJ[A-Za-z0-9_-]{10,}(?:\.[A-Za-z0-9_-]{10,}){2,5}\b)");

	QRegularExpression jsonTokenRegex(
		R"((?i)\b(?:access_token|refresh_token|id_token|token)\b\s*[:=]\s*(?:\\?")?(?:[^"\\]|\\.){16,}(?:\\?")?)");

	QRegularExpression tokenRegex(
		R"(\b[A-Za-z0-9_-]{16,}(?:\.[A-Za-z0-9_-]{8,}){2,6}\b)");

	QRegularExpression uuidRegex(
		R"(\b[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}\b)");

	QRegularExpression hexRegex(R"(\b[0-9a-fA-F]{32,64}\b)");

	QRegularExpression idRegex(R"(\b\d{10,20}\b)");

	auto matchAuthLineRegex = authLineRegex.globalMatch(logContent);
	while (matchAuthLineRegex.hasNext()) {
		auto m = matchAuthLineRegex.next();
		QString authLine = m.captured(0);
		if (authLine.length() > 10) {
			filter[authLine] = "<CENSOR_AUTHLINE>";
		}
	}

	auto matchBearerHeader = bearerHeader.globalMatch(logContent);
	while (matchBearerHeader.hasNext()) {
		auto m = matchBearerHeader.next();
		QString bearer = m.captured(0);
		if (bearer.length() > 10) {
			filter[bearer] = "<CENSOR_BEARER>";
		}
	}

	auto matchJwtFirstRegex = jwtFirstRegex.globalMatch(logContent);
	while (matchJwtFirstRegex.hasNext()) {
		auto m = matchJwtFirstRegex.next();
		QString jwt = m.captured(0);
		if (jwt.length() > 10) {
			filter[jwt] = "<CENSOR_JWT>";
		}
	}

	auto matchJwtSecondRegex = jwtSecondRegex.globalMatch(logContent);
	while (matchJwtSecondRegex.hasNext()) {
		auto m = matchJwtSecondRegex.next();
		QString jwt = m.captured(0);
		if (jwt.length() > 10) {
			filter[jwt] = "<CENSOR_JWT>";
		}
	}

	auto matchJsonTokenRegex = jsonTokenRegex.globalMatch(logContent);
	while (matchJsonTokenRegex.hasNext()) {
		auto m = matchJsonTokenRegex.next();
		QString json = m.captured(0);
		if (json.length() > 10) {
			filter[json] = "<CENSOR_JSONTOKEN>";
		}
	}

	auto matchTokenRegex = tokenRegex.globalMatch(logContent);
	while (matchTokenRegex.hasNext()) {
		auto m = matchTokenRegex.next();
		QString token = m.captured(0);
		if (token.length() > 10) {
			filter[token] = "<CENSOR_TOKEN>";
		}
	}

	auto matchUuidRegex = uuidRegex.globalMatch(logContent);
	while (matchUuidRegex.hasNext()) {
		auto m = matchUuidRegex.next();
		QString uuid = m.captured(0);
		if (uuid.length() > 10) {
			filter[uuid] = "<CENSOR_UUID>";
		}
	}

	auto matchHexRegex = hexRegex.globalMatch(logContent);
	while (matchHexRegex.hasNext()) {
		auto m = matchHexRegex.next();
		QString hex = m.captured(0);
		if (hex.length() > 10) {
			filter[hex] = "<CENSOR_HEX>";
		}
	}

	auto matchIdRegex = idRegex.globalMatch(logContent);
	while (matchIdRegex.hasNext()) {
		auto m = matchIdRegex.next();
		QString id = m.captured(0);
		if (id.length() > 10) {
			filter[id] = "<CENSOR_ID>";
		}
	}

	logContent = censorText(logContent, filter);

	// Upload to paste.ee
	QNetworkAccessManager nam;
	QString pasteLink = uploadToPasteEE(&nam, logContent, apiKey);
	if (pasteLink.isEmpty()) {
		QMessageBox::warning(
			nullptr, "MeshMC Crash Reporter",
			"Failed to upload crash log to paste.ee.\n"
			"Please manually share the log file located at:\n" +
				QDir(logDir).absoluteFilePath(
					QString("%1-0.log").arg(baseName)));
		return 1;
	}

	// Show the crash dialog
	CrashReportDialog dialog(pasteLink, logContent);
	dialog.exec();

	return 0;
}
