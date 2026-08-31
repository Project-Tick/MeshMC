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

#include <QTimer>
#include <QNetworkReply>

#include "katabasis/Reply.h"

namespace Katabasis
{

	Reply::Reply(QNetworkReply* r, int timeOut, QObject* parent)
		: QTimer(parent), reply(r)
	{
		setSingleShot(true);
		connect(this, &Reply::timeout, this, &Reply::onTimeOut,
				Qt::QueuedConnection);
		start(timeOut);
	}

	void Reply::onTimeOut()
	{
		timedOut = true;
		reply->abort();
	}

	// ----------------------------

	ReplyList::~ReplyList()
	{
		foreach (Reply* timedReply, replies_) {
			delete timedReply;
		}
	}

	void ReplyList::add(QNetworkReply* reply, int timeOut)
	{
		if (reply && ignoreSslErrors()) {
			reply->ignoreSslErrors();
		}
		add(new Reply(reply, timeOut));
	}

	void ReplyList::add(Reply* reply)
	{
		replies_.append(reply);
	}

	void ReplyList::remove(QNetworkReply* reply)
	{
		Reply* o2Reply = find(reply);
		if (o2Reply) {
			o2Reply->stop();
			(void)replies_.removeOne(o2Reply);
		}
	}

	Reply* ReplyList::find(QNetworkReply* reply)
	{
		foreach (Reply* timedReply, replies_) {
			if (timedReply->reply == reply) {
				return timedReply;
			}
		}
		return 0;
	}

	bool ReplyList::ignoreSslErrors()
	{
		return ignoreSslErrors_;
	}

	void ReplyList::setIgnoreSslErrors(bool ignoreSslErrors)
	{
		ignoreSslErrors_ = ignoreSslErrors;
	}

} // namespace Katabasis
