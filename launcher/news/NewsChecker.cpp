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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include "NewsChecker.h"

#include <QByteArray>
#include <QDomDocument>

#include <QDebug>

#include <algorithm>

NewsChecker::NewsChecker(shared_qobject_ptr<QNetworkAccessManager> network,
						 const QString& feedUrl)
	: NewsChecker(network, QStringList{feedUrl})
{
}

NewsChecker::NewsChecker(shared_qobject_ptr<QNetworkAccessManager> network,
						 const QStringList& feedUrls)
{
	m_network = network;

	// Feed 0 has to stay feed 0 whatever the caller passed, because the
	// indices are handed out to the news dialog and to plugins.
	for (const QString& url : feedUrls) {
		const QString trimmed = url.trimmed();
		if (trimmed.isEmpty())
			continue;

		bool seen = false;
		for (const auto& feed : m_feeds) {
			if (feed.url == trimmed) {
				seen = true;
				break;
			}
		}
		if (seen)
			continue;

		Feed feed;
		feed.url = trimmed;
		m_feeds.append(feed);
	}
}

QStringList NewsChecker::feedUrls() const
{
	QStringList urls;
	urls.reserve(m_feeds.size());
	for (const auto& feed : m_feeds) {
		urls.append(feed.url);
	}
	return urls;
}

void NewsChecker::reloadNews()
{
	// Start a netjob per feed and call rssDownloadFinished() as each one
	// lands.
	if (isLoadingNews()) {
		qDebug()
			<< "Ignored request to reload news. Currently reloading already.";
		return;
	}

	if (m_feeds.isEmpty()) {
		qDebug() << "No news feeds configured.";
		succeed();
		return;
	}

	qDebug() << "Reloading news from" << m_feeds.size() << "feed(s).";

	m_primaryError.clear();
	m_pendingFeeds = m_feeds.size();

	// Every job is created before any of them starts: a job that fails
	// synchronously would otherwise settle the load while later feeds
	// have not even been counted in yet.
	for (int i = 0; i < m_feeds.size(); i++) {
		auto& feed = m_feeds[i];
		feed.data.clear();

		NetJob* job = new NetJob("News RSS Feed", m_network);
		job->addNetAction(Net::Download::makeByteArray(feed.url, &feed.data));
		QObject::connect(job, &NetJob::succeeded, this,
						 [this, i] { rssDownloadFinished(i); });
		QObject::connect(job, &NetJob::failed, this,
						 [this, i](QString reason) {
							 rssDownloadFailed(i, reason);
						 });
		feed.job.reset(job);
	}

	for (auto& feed : m_feeds) {
		feed.job->start();
	}
}

void NewsChecker::rssDownloadFinished(int feedIndex)
{
	if (feedIndex < 0 || feedIndex >= m_feeds.size())
		return;

	auto& feed = m_feeds[feedIndex];

	// Parse the XML file and process the RSS feed entries.
	qDebug() << "Finished loading RSS feed" << feed.url;

	feed.job.reset();
	feed.entries.clear();

	QDomDocument doc;
	{
		// Stuff to store error info in.
		QString errorMsg = "Unknown error.";
		int errorLine = -1;
		int errorCol = -1;

		// Parse the XML.
		if (!doc.setContent(feed.data, false, &errorMsg, &errorLine,
							&errorCol)) {
			QString fullErrorMsg =
				QString("Error parsing RSS feed XML. %1 at %2:%3.")
					.arg(errorMsg)
					.arg(errorLine)
					.arg(errorCol);
			feed.data.clear();
			rssDownloadFailed(feedIndex, fullErrorMsg);
			return;
		}
		feed.data.clear();
	}

	// If the parsing succeeded, read it.
	QDomNodeList items = doc.elementsByTagName("item");
	for (int i = 0; i < items.length(); i++) {
		QDomElement element = items.at(i).toElement();
		NewsEntryPtr entry;
		entry.reset(new NewsEntry());
		QString errorMsg = "An unknown error occurred.";
		if (NewsEntry::fromXmlElement(element, entry.get(), &errorMsg)) {
			entry->feedIndex = feedIndex;
			qDebug() << "Loaded news entry" << entry->title;
			feed.entries.append(entry);
		} else {
			qWarning() << "Failed to load news entry at index" << i << ":"
					   << errorMsg;
		}
	}

	feedSettled();
}

void NewsChecker::rssDownloadFailed(int feedIndex, QString reason)
{
	if (feedIndex < 0 || feedIndex >= m_feeds.size())
		return;

	auto& feed = m_feeds[feedIndex];
	feed.job.reset();
	feed.data.clear();
	// Drop whatever this feed contributed last time: showing entries
	// from a feed that just failed to load is worse than showing none.
	feed.entries.clear();

	const QString message =
		tr("Failed to load news RSS feed:\n%1").arg(reason);

	if (feedIndex == 0) {
		// The primary feed is the one the news bar speaks for.
		m_primaryError = message;
	} else {
		// An extra feed is additive. Losing it is not worth blanking
		// the news bar over.
		qWarning() << "Failed to load extra news feed" << feed.url << ":"
				   << reason;
	}

	feedSettled();
}

void NewsChecker::feedSettled()
{
	if (m_pendingFeeds > 0)
		m_pendingFeeds--;
	if (m_pendingFeeds > 0)
		return;

	if (!m_primaryError.isEmpty()) {
		fail(m_primaryError);
		return;
	}
	succeed();
}

QList<NewsEntryPtr> NewsChecker::getNewsEntries() const
{
	QList<NewsEntryPtr> merged;
	for (const auto& feed : m_feeds) {
		merged.append(feed.entries);
	}

	// Newest first, so the news bar's "latest headline" really is the
	// latest one across every feed rather than whichever feed happens
	// to come first. Undated entries sink to the bottom instead of
	// jumping to the top on an invalid QDateTime comparison.
	std::stable_sort(merged.begin(), merged.end(),
					 [](const NewsEntryPtr& a, const NewsEntryPtr& b) {
						 if (a->pubDate.isValid() != b->pubDate.isValid())
							 return a->pubDate.isValid();
						 return a->pubDate > b->pubDate;
					 });
	return merged;
}

bool NewsChecker::isLoadingNews() const
{
	for (const auto& feed : m_feeds) {
		if (feed.job.get() != nullptr)
			return true;
	}
	return false;
}

QString NewsChecker::getLastLoadErrorMsg() const
{
	return m_lastLoadError;
}

void NewsChecker::succeed()
{
	m_lastLoadError = "";
	m_loadedNews = true;
	qDebug() << "News loading succeeded.";
	emit newsLoaded();
}

void NewsChecker::fail(const QString& errorMsg)
{
	m_lastLoadError = errorMsg;
	qDebug() << "Failed to load news:" << errorMsg;
	emit newsLoadingFailed(errorMsg);
}
