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
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QByteArray>

#include "katabasis/Reply.h"

/// Makes authentication requests.
class AuthRequest : public QObject
{
	Q_OBJECT

  public:
	explicit AuthRequest(QObject* parent = 0);
	~AuthRequest();

  public slots:
	void get(const QNetworkRequest& req, int timeout = 60 * 1000);
	void post(const QNetworkRequest& req, const QByteArray& data,
			  int timeout = 60 * 1000);

  signals:

	/// Emitted when a request has been completed or failed.
	void finished(QNetworkReply::NetworkError error, QByteArray data,
				  QList<QNetworkReply::RawHeaderPair> headers);

	/// Emitted when an upload has progressed.
	void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

  protected slots:

	/// Handle request finished.
	void onRequestFinished();

	/// Handle request error.
	void onRequestError(QNetworkReply::NetworkError error);

	/// Handle ssl errors.
	void onSslErrors(QList<QSslError> errors);

	/// Finish the request, emit finished() signal.
	void finish();

	/// Handle upload progress.
	void onUploadProgress(qint64 uploaded, qint64 total);

  public:
	QNetworkReply::NetworkError error_;
	int httpStatus_ = 0;
	QString errorString_;

  protected:
	void setup(const QNetworkRequest& request,
			   QNetworkAccessManager::Operation operation,
			   const QByteArray& verb = QByteArray());

	enum Status { Idle, Requesting, ReRequesting };

	QNetworkRequest request_;
	QByteArray data_;
	QNetworkReply* reply_;
	Status status_;
	QNetworkAccessManager::Operation operation_;
	QUrl url_;
	Katabasis::ReplyList timedReplies_;

	QTimer* timer_;
};
