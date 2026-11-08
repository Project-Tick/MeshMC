/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include "NewsFeed.h"

#include <QDateTime>

#include <KLocalizedString>

#include <syndication/syndication.h>

namespace News
{
	bool parseFeed(const QByteArray& data, const QString& url, int feedIndex,
				   QList<NewsEntryPtr>* entries, QString* error)
	{
		const Syndication::FeedPtr feed =
			Syndication::parse(Syndication::DocumentSource(data, url));
		if (!feed) {
			*error = QStringLiteral("this is not an RSS or an Atom feed");
			return false;
		}

		entries->clear();
		for (const Syndication::ItemPtr& item : feed->items()) {
			auto entry = std::make_shared<NewsEntry>();

			// The title of an item is HTML as far as Syndication is
			// concerned ("Fish &amp; Chips"); a NewsEntry has plain text
			// there, which the news dialog escapes itself.
			const QString title = Syndication::resolveEntities(item->title());
			if (!title.isEmpty()) {
				entry->title = title;
			}

			// An RSS item says its text in the description, unless it has a
			// content of its own; an Atom entry, the other way round.
			const QString content = item->content().isEmpty()
										? item->description()
										: item->content();
			if (!content.isEmpty()) {
				entry->content = content;
			}

			entry->link = item->link();

			const auto authors = item->authors();
			if (!authors.isEmpty() && !authors.first()->name().isEmpty()) {
				entry->author =
					Syndication::resolveEntities(authors.first()->name());
			}

			const time_t published = item->datePublished() != 0
										 ? item->datePublished()
										 : item->dateUpdated();
			entry->pubDate = published != 0
								 ? QDateTime::fromSecsSinceEpoch(published)
								 : QDateTime();

			entry->feedIndex = feedIndex;
			entries->append(entry);
		}
		return true;
	}
} // namespace News
