/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * SearchIndex — tiny prefix-trie over article titles. Kept separate
 * from each bundle so it can be re-built when the user adds/removes
 * bundles at runtime.
 */

#pragma once

#include "plugin/sdk/mmco_sdk.h"

class WikiBundle;

class SearchIndex
{
  public:
	void reset();
	void addBundle(WikiBundle* bundle);

	/* Return up to `limit` (bundle*, slug) pairs whose title contains
	 * `query` (case-insensitive). */
	struct Hit {
		WikiBundle* bundle;
		QString slug;
		QString title;
	};
	QList<Hit> search(const QString& query, int limit = 50) const;

  private:
	struct Entry {
		WikiBundle* bundle;
		QString slug;
		QString title;
	};
	QList<Entry> m_entries;
};
