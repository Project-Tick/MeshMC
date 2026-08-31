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

#include <QList>
#include <QTimer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QByteArray>

namespace Katabasis
{

	constexpr int defaultTimeout = 30 * 1000;

	/// A network request/reply pair that can time out.
	class Reply : public QTimer
	{
		Q_OBJECT

	  public:
		Reply(QNetworkReply* reply, int timeOut = defaultTimeout,
			  QObject* parent = 0);

	  signals:
		void error(QNetworkReply::NetworkError);

	  public slots:
		/// When time out occurs, the QNetworkReply's error() signal is
		/// triggered.
		void onTimeOut();

	  public:
		QNetworkReply* reply;
		bool timedOut = false;
	};

	/// List of O2Replies.
	class ReplyList
	{
	  public:
		ReplyList()
		{
			ignoreSslErrors_ = false;
		}

		/// Destructor.
		/// Deletes all O2Reply instances in the list.
		virtual ~ReplyList();

		/// Create a new O2Reply from a QNetworkReply, and add it to this list.
		void add(QNetworkReply* reply, int timeOut = defaultTimeout);

		/// Add an O2Reply to the list, while taking ownership of it.
		void add(Reply* reply);

		/// Remove item from the list that corresponds to a QNetworkReply.
		void remove(QNetworkReply* reply);

		/// Find an O2Reply in the list, corresponding to a QNetworkReply.
		/// @return Matching O2Reply or NULL.
		Reply* find(QNetworkReply* reply);

		bool ignoreSslErrors();
		void setIgnoreSslErrors(bool ignoreSslErrors);

	  protected:
		QList<Reply*> replies_;
		bool ignoreSslErrors_;
	};

} // namespace Katabasis
