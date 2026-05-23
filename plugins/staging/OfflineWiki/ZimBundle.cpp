/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ZimBundle.h"

#include <QtEndian>

namespace
{
	constexpr quint32 ZIM_MAGIC = 0x44D495A; // "ZIMD" little-endian
}

bool ZimBundle::open(const QString& path)
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly))
		return false;
	if (!readHeader(f))
		return false;
	if (!readTitlePointerList(f))
		return false;
	m_path = path;
	m_name = QFileInfo(path).completeBaseName();
	m_open = true;
	return true;
}

bool ZimBundle::readHeader(QFile& f)
{
	// Header layout (76 bytes):
	//   uint32 magicNumber
	//   uint16 majorVersion
	//   uint16 minorVersion
	//   uuid   (16 bytes)
	//   uint32 articleCount
	//   uint32 clusterCount
	//   uint64 urlPtrPos
	//   uint64 titlePtrPos
	//   uint64 clusterPtrPos
	//   uint64 mimeListPos
	//   uint32 mainPage
	//   uint32 layoutPage
	//   uint64 checksumPos
	if (f.size() < 80)
		return false;
	QByteArray hdr = f.read(76);
	if (hdr.size() != 76)
		return false;

	auto readU32 = [&](int off) {
		return qFromLittleEndian<quint32>(hdr.mid(off, 4).constData());
	};
	auto readU64 = [&](int off) {
		return qFromLittleEndian<quint64>(hdr.mid(off, 8).constData());
	};

	quint32 magic = readU32(0);
	if (magic != ZIM_MAGIC)
		return false;
	m_articleCount = readU32(24);
	m_urlPtrPos = readU64(32);
	m_titlePtrPos = readU64(40);
	return true;
}

bool ZimBundle::readTitlePointerList(QFile& f)
{
	// Title pointer list = articleCount * uint32 indices into the URL
	// pointer list, but for now we only need the article-ID ordering;
	// the URL itself is the slug. To keep this stub small we
	// short-circuit and present articles by their ZIM-internal id.
	m_titles.clear();
	if (!f.seek(m_titlePtrPos))
		return false;
	for (quint32 i = 0; i < m_articleCount; ++i) {
		quint32 idx;
		if (f.read(reinterpret_cast<char*>(&idx), 4) != 4)
			return false;
		idx = qFromLittleEndian(idx);
		m_titles.append(QStringLiteral("article-%1").arg(idx));
	}
	return true;
}

WikiNavNode ZimBundle::buildNav() const
{
	WikiNavNode root;
	root.title = m_name;
	for (const auto& t : m_titles) {
		WikiNavNode n;
		n.title = t;
		n.slug = t;
		root.children.append(n);
	}
	return root;
}

QString ZimBundle::renderArticleHtml(const QString& slug) const
{
	return QStringLiteral(
			   "<h2>Article '%1' from ZIM bundle</h2>"
			   "<p>This OfflineWiki build can index a ZIM file's article list "
			   "but does not yet decompress article bodies. Use a directory "
			   "bundle for now, or install <a "
			   "href=\"https://kiwix.org\">Kiwix</a> "
			   "to read the full content.</p>")
		.arg(slug.toHtmlEscaped());
}

QStringList ZimBundle::searchTitles(const QString& query, int limit) const
{
	QStringList out;
	for (const auto& t : m_titles) {
		if (t.contains(query, Qt::CaseInsensitive)) {
			out.append(t);
			if (out.size() >= limit)
				break;
		}
	}
	return out;
}
