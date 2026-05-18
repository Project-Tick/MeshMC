/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ZimBundle — minimal reader for the Kiwix ZIM file format.
 *
 * The ZIM specification (https://openzim.org/wiki/ZIM_file_format) is
 * a packed, dictionary-compressed bundle. Implementing a full reader
 * in-tree is a much larger project than the rest of this plugin, so
 * this implementation supports the structural minimum:
 *
 *   • parse the ZIM header (magic, URL/title pointer counts)
 *   • read the URL pointer list and the matching title list so we can
 *     present article titles in the nav
 *   • report all articles with a placeholder body that asks the user
 *     to install Kiwix (or wait for full-content support)
 *
 * It's deliberately conservative — we don't want to ship subtly broken
 * decompression. Future revisions can add Zstd cluster decoding and
 * link rewriting. Until then the bundle still surfaces titles, which
 * makes the search UI useful for "does my dump have an article about
 * X?" questions.
 */

#pragma once

#include "WikiBundle.h"

class ZimBundle : public WikiBundle
{
  public:
	bool open(const QString& path);

	QString name() const override { return m_name; }
	QString format() const override { return QStringLiteral("zim"); }
	QString rootPath() const override { return m_path; }
	bool isOpen() const override { return m_open; }
	int articleCount() const override { return m_titles.size(); }

	WikiNavNode buildNav() const override;
	QString renderArticleHtml(const QString& slug) const override;
	QStringList searchTitles(const QString& query, int limit) const override;

  private:
	bool readHeader(QFile& f);
	bool readTitlePointerList(QFile& f);

	QString m_path;
	QString m_name;
	bool m_open = false;
	quint32 m_articleCount = 0;
	quint64 m_urlPtrPos = 0;
	quint64 m_titlePtrPos = 0;
	QList<QString> m_titles; // ordered as in the file
};
