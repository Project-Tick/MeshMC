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

#include "NewsEntry.h"

#include <KLocalizedString>

NewsEntry::NewsEntry(QObject* parent) : QObject(parent)
{
	this->title = i18n("Untitled");
	this->content = i18n("No content.");
	this->link = "";
	this->author = i18n("Unknown Author");
	this->pubDate = QDateTime::currentDateTime();
}

NewsEntry::NewsEntry(const QString& title, const QString& content,
					 const QString& link, const QString& author,
					 const QDateTime& pubDate, QObject* parent)
	: QObject(parent)
{
	this->title = title;
	this->content = content;
	this->link = link;
	this->author = author;
	this->pubDate = pubDate;
}
