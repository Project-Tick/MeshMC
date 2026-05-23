/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "SearchIndex.h"
#include "WikiBundle.h"

void SearchIndex::reset()
{
	m_entries.clear();
}

void SearchIndex::addBundle(WikiBundle* bundle)
{
	if (!bundle)
		return;
	// Walk the bundle's nav tree to enumerate every (slug, title).
	std::function<void(const WikiNavNode&)> walk = [&](const WikiNavNode& n) {
		if (!n.slug.isEmpty()) {
			Entry e;
			e.bundle = bundle;
			e.slug = n.slug;
			e.title = n.title;
			m_entries.append(e);
		}
		for (const auto& c : n.children)
			walk(c);
	};
	walk(bundle->buildNav());
}

QList<SearchIndex::Hit> SearchIndex::search(const QString& query,
											int limit) const
{
	QList<Hit> out;
	if (query.isEmpty())
		return out;
	for (const auto& e : m_entries) {
		if (e.title.contains(query, Qt::CaseInsensitive)) {
			out.append({e.bundle, e.slug, e.title});
			if (out.size() >= limit)
				break;
		}
	}
	return out;
}
