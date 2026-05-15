/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * WikiBundle — common interface every bundle format implements.
 *
 * Articles are addressed by an opaque `slug` string (per-bundle).
 * Plugins layer a SearchIndex on top to provide title-search;
 * full-text search is bundle-specific (ZIM has its own xapian-backed
 * index, the directory bundle uses a tiny in-memory inverted list).
 */

#pragma once

#include "plugin/sdk/mmco_sdk.h"

struct WikiNavNode {
	QString title;
	QString slug;  // empty if this node is a category
	QList<WikiNavNode> children;
};

class WikiBundle
{
  public:
	virtual ~WikiBundle() = default;

	virtual QString name() const = 0;
	virtual QString format() const = 0; // "directory" / "zim" / …
	virtual QString rootPath() const = 0;
	virtual bool isOpen() const = 0;
	virtual int articleCount() const = 0;

	/* Build the side-pane navigation. The root node's title is the
	 * bundle name. */
	virtual WikiNavNode buildNav() const = 0;

	/* Resolve a slug to fully-rendered HTML (with inline image data:
	 * URIs where possible). Empty result = not found. */
	virtual QString renderArticleHtml(const QString& slug) const = 0;

	/* Return up to `limit` slugs whose titles contain `query`
	 * (case-insensitive substring match). */
	virtual QStringList searchTitles(const QString& query, int limit = 50) const = 0;

	/* Optional full-text search. Default implementation falls back to
	 * a substring scan over rendered article text — slow but correct. */
	virtual QStringList searchFullText(const QString& query,
									   int limit = 50) const;
};

/* Try to open the bundle at `path`. Returns nullptr on failure. The
 * factory peeks at `path` to decide which concrete bundle to instantiate. */
WikiBundle* openBundle(const QString& path);
