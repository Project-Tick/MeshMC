/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

#include <QDialog>
#include <QList>

#include "news/NewsEntry.h"

class NewsChecker;
class QLabel;
class QListWidget;
class QPushButton;
class QTextBrowser;

class NewsViewerDialog : public QDialog
{
    Q_OBJECT

public:
    NewsViewerDialog(NewsChecker& checker, bool showArticleList, QWidget* parent);

private:
    void reloadEntries();
    void showArticle(int row);
    void setArticleListVisible(bool visible);

    NewsChecker& m_checker;
    QList<NewsEntryPtr> m_entries;
    QLabel* m_title;
    QListWidget* m_articles;
    QTextBrowser* m_body;
    QPushButton* m_toggleList;
    QPushButton* m_openArticle;
};
