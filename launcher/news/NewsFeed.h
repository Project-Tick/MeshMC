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

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include "NewsEntry.h"

namespace News
{
	/*!
	 * Reads the entries of a news feed, which is an RSS or an Atom document
	 * (KDE Frameworks' Syndication tells which, and reads either).
	 *
	 * Every entry gets \p feedIndex. A date is taken as it is given, with its
	 * time zone; an entry without a date has an invalid one, which makes it
	 * the oldest. An entry that has no title, content or author gets the
	 * words NewsEntry has for that.
	 *
	 * \param url where the document came from, which a feed may use to
	 *        resolve addresses that are relative to it.
	 * \return false if \p data is not a feed; \p error then says what is wrong.
	 */
	bool parseFeed(const QByteArray& data, const QString& url, int feedIndex,
				   QList<NewsEntryPtr>* entries, QString* error);
} // namespace News
