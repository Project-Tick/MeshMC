/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "WikiBundle.h"
#include "DirectoryBundle.h"
#include "ZimBundle.h"

QStringList WikiBundle::searchFullText(const QString& query, int limit) const
{
	QStringList out;
	auto titleHits = searchTitles(query, limit);
	for (const auto& slug : titleHits) {
		QString html = renderArticleHtml(slug);
		QString plain = html;
		// Strip tags, decode entities very lightly.
		plain.remove(QRegularExpression("<[^>]+>"));
		if (plain.contains(query, Qt::CaseInsensitive))
			out.append(slug);
		if (out.size() >= limit)
			break;
	}
	return out;
}

WikiBundle* openBundle(const QString& path)
{
	QFileInfo fi(path);
	if (fi.isDir()) {
		auto* b = new DirectoryBundle();
		if (b->open(path))
			return b;
		delete b;
		return nullptr;
	}
	if (fi.isFile() && path.endsWith(QStringLiteral(".zim"),
									 Qt::CaseInsensitive)) {
		auto* b = new ZimBundle();
		if (b->open(path))
			return b;
		delete b;
		return nullptr;
	}
	return nullptr;
}
