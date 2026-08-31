/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * WikiRepoBundle — the one and only wiki backend: a cloned Git wiki
 * repository, in the flat layout GitLab/GitHub wikis use:
 *
 *   <root>/Page-Name.md         — one Markdown file per page; the slug
 *                                 is the file's base name (spaces are
 *                                 written as dashes in the file name).
 *   <root>/Home.md              — landing page (falls back to the first
 *                                 page alphabetically if absent).
 *   <root>/_Sidebar.md          — ignored as an article (meta page).
 *   <root>/_Footer.md           — optional, appended to every page.
 *   <root>/images/…             — media referenced with relative paths.
 *
 * The page list is discovered by scanning *.md. It also rewrites the
 * two wiki link styles into an internal "wiki:" URL scheme so the
 * viewer can navigate between pages:
 *
 *   [[Page Name]]      -> <a href="wiki:Page-Name">Page Name</a>
 *   [text](Page-Name)  -> <a href="wiki:Page-Name">text</a>
 *
 * (links that are already absolute — http(s):, mailto:, #anchors,
 * or existing files — are left untouched.)
 */

#pragma once

#include "plugin/sdk/mmco_cxx_sdk.hpp"

class WikiRepoBundle
{
  public:
	/* A single navigable page: its slug ("Page-Name") and display
	 * title ("Page Name"). */
	struct Entry {
		QString slug;
		QString title;
	};

	bool open(const QString& dir);

	QString name() const
	{
		return m_name;
	}
	QString rootPath() const
	{
		return m_root;
	}
	bool isOpen() const
	{
		return !m_root.isEmpty();
	}
	int articleCount() const
	{
		return m_order.size();
	}

	/* Navigation list: Home first (if present), then alphabetical. */
	QList<Entry> nav() const;

	/* Resolve a slug to fully-rendered HTML (internal links rewritten to
	 * the wiki: scheme, relative images resolved). Empty if not found. */
	QString renderArticleHtml(const QString& slug) const;

	/* Pages whose title contains `query` (case-insensitive substring). */
	QList<Entry> searchTitles(const QString& query, int limit = 200) const;

	/* Slug helpers. */
	static QString slugFromFileName(const QString& fileName);
	static QString titleFromSlug(const QString& slug);

  private:
	struct Article {
		QString slug;	 // "Page-Name"
		QString title;	 // "Page Name"
		QString relFile; // "Page-Name.md"
	};

	QString rewriteLinks(const QString& markdown) const;

	QString m_name;
	QString m_root;
	QHash<QString, Article> m_articles; // slug -> article
	QList<Article> m_order;				// alphabetical
	bool m_hasFooter = false;
};
