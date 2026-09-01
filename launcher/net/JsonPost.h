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
#include <QNetworkReply>
#include <QString>
#include <QUrl>

#include <memory>

#include "tasks/Task.h"

namespace Net
{
	/*
	 * One JSON request that has to be a POST, as a task.
	 *
	 * Net::Download covers everything this launcher fetches, and it is a
	 * GET: a URL, a sink, a cache entry, validators. Most of that is
	 * meaningless for an API call whose question does not fit in a URL -
	 * "which versions own these two hundred fingerprints?" - and one of
	 * those calls is unavoidable, because CurseForge offers no other way
	 * to identify a file on disk.
	 *
	 * So this is deliberately the smaller thing: no caching (the answer
	 * is about the files the user has right now), no sink (the reply is a
	 * JSON document that the caller parses and discards), no partial
	 * progress (the body is a few kilobytes either way).
	 *
	 * The CurseForge credential is attached under exactly the condition
	 * Net::Download uses, and for the same reason: the key belongs to
	 * that API's host and to nowhere else, so the rule lives next to the
	 * host check rather than at the call sites, where one omission would
	 * leak it or one typo would silently drop it and turn every reply
	 * into a 403.
	 */
	class JsonPost : public Task
	{
		Q_OBJECT
	  public:
		JsonPost(QString name, QUrl url, QByteArray body,
				 QObject* parent = nullptr);
		~JsonPost() override = default;

		/* The reply body. Only meaningful after the task succeeded. */
		const QByteArray& response() const
		{
			return m_response;
		}

		bool canAbort() const override
		{
			return true;
		}
		bool abort() override;

	  protected:
		void executeTask() override;

	  private slots:
		void requestFinished();

	  private:
		const QString m_name;
		const QUrl m_url;
		const QByteArray m_body;
		QByteArray m_response;

		/* Latched by abort() so the finished handler - which Qt still
		 * delivers for an aborted reply - reports the verdict the user
		 * asked for rather than a network error on top of it. */
		bool m_aborted = false;

		std::unique_ptr<QNetworkReply> m_reply;
	};
} // namespace Net
