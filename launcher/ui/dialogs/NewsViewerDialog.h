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

#include <QDialog>
#include <QList>
#include <QPointer>
#include <QString>

#include "news/NewsEntry.h"

class NewsChecker;

class QLabel;
class QListWidget;
class QPushButton;
class QTextBrowser;

/*
 * NewsViewerDialog — reads the launcher's news feeds.
 *
 * The layout follows Prism Launcher's NewsDialog: article list on the
 * left, the headline as a link over the article body on the right, and
 * a row at the bottom with the list toggle and Close. Keeping the two
 * launchers' news windows recognisably the same is deliberate — users
 * move between them.
 *
 * Two ways in, differing only in whether the list starts open:
 *   list visible — "More news": pick from every entry of every feed.
 *   list hidden  — the news bar headline: straight to the latest post.
 *
 * Entries come from MainWindow's NewsChecker, so the dialog never
 * downloads anything itself; it repopulates whenever the checker
 * reports a load has finished.
 *
 * This was the NewsViewer .mmco plugin before it moved into core. The
 * MMCO news_* C API is still there for out-of-tree plugins — it just
 * answers from the same NewsChecker now.
 */
class NewsViewerDialog : public QDialog
{
	Q_OBJECT

  public:
	explicit NewsViewerDialog(NewsChecker* checker, QWidget* parent = nullptr);
	~NewsViewerDialog() override = default;

	/* Populate from the checker. If listVisible is false the article
	 * list starts collapsed and the latest post is shown on its own. */
	void loadEntries(bool listVisible = true);
	bool isSidebarVisible() const
	{
		return !m_articleListHidden;
	}

  private slots:
	void selectedArticleChanged(int row);
	void toggleArticleList();
	void onNewsLoaded();

  private:
	static QString renderContent(const QString& raw);
	void setArticleListHidden(bool hidden);
	void showPlaceholder(const QString& message);
	/* Human-readable name for the feed an entry came from. */
	QString feedLabel(int feedIndex) const;

	QPointer<NewsChecker> m_checker;

	QListWidget* m_articleList = nullptr;
	QLabel* m_articleTitleLabel = nullptr;
	QTextBrowser* m_articleContent = nullptr;
	QPushButton* m_toggleListButton = nullptr;
	QPushButton* m_closeButton = nullptr;

	QList<NewsEntryPtr> m_entries;
	bool m_articleListHidden = false;
};
