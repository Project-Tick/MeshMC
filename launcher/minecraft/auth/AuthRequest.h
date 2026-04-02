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
