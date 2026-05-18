/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * DirectoryBundle — wiki backed by a plain directory tree.
 *
 * Layout:
 *   index.json   — { name, articles: [{slug, title, file, category?}], nav?: [...] }
 *   *.md         — markdown articles, addressed by their `file` field
 *   media/       — images, fonts, etc. referenced from articles
 *
 * `category` is optional; if present the bundle auto-builds a
 * two-level nav (Category → Articles). If `nav` is supplied
 * explicitly, that takes priority and may be arbitrary deep.
 */

#pragma once

#include "WikiBundle.h"

class DirectoryBundle : public WikiBundle
{
  public:
	bool open(const QString& dir);

	QString name() const override { return m_name; }
	QString format() const override { return QStringLiteral("directory"); }
	QString rootPath() const override { return m_root; }
	bool isOpen() const override { return !m_root.isEmpty(); }
	int articleCount() const override { return m_articles.size(); }

	WikiNavNode buildNav() const override;
	QString renderArticleHtml(const QString& slug) const override;
	QStringList searchTitles(const QString& query, int limit) const override;

  private:
	struct Article {
		QString slug;
		QString title;
		QString relFile; // path relative to bundle root
		QString category;
	};

	QString m_name;
	QString m_root;
	QHash<QString, Article> m_articles;
	QList<Article> m_order;
	QJsonArray m_navJson; // explicit nav, if provided
};
