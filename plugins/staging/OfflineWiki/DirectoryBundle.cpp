/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "DirectoryBundle.h"

#include <QTextDocument>

namespace
{
	WikiNavNode parseNavJson(const QJsonArray& arr)
	{
		WikiNavNode root;
		for (auto v : arr) {
			auto obj = v.toObject();
			WikiNavNode n;
			n.title = obj.value("title").toString();
			n.slug = obj.value("slug").toString();
			auto children = obj.value("children").toArray();
			if (!children.isEmpty()) {
				WikiNavNode sub = parseNavJson(children);
				n.children = sub.children;
			}
			root.children.append(n);
		}
		return root;
	}
} // namespace

bool DirectoryBundle::open(const QString& dir)
{
	QFile f(QDir(dir).filePath("index.json"));
	if (!f.open(QIODevice::ReadOnly))
		return false;
	auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject())
		return false;

	auto root = doc.object();
	m_name = root.value("name").toString();
	if (m_name.isEmpty())
		m_name = QFileInfo(dir).fileName();
	m_root = dir;
	m_navJson = root.value("nav").toArray();

	for (auto v : root.value("articles").toArray()) {
		auto obj = v.toObject();
		Article a;
		a.slug = obj.value("slug").toString();
		a.title = obj.value("title").toString();
		a.relFile = obj.value("file").toString();
		a.category = obj.value("category").toString();
		if (a.slug.isEmpty() || a.relFile.isEmpty())
			continue;
		m_articles.insert(a.slug, a);
		m_order.append(a);
	}
	return !m_articles.isEmpty();
}

WikiNavNode DirectoryBundle::buildNav() const
{
	WikiNavNode root;
	root.title = m_name;

	if (!m_navJson.isEmpty()) {
		auto explicitNav = parseNavJson(m_navJson);
		root.children = explicitNav.children;
		return root;
	}

	QHash<QString, WikiNavNode*> categories;
	WikiNavNode uncategorised;
	uncategorised.title = QObject::tr("Uncategorised");

	for (const auto& a : m_order) {
		WikiNavNode leaf;
		leaf.title = a.title;
		leaf.slug = a.slug;
		if (a.category.isEmpty()) {
			uncategorised.children.append(leaf);
			continue;
		}
		if (!categories.contains(a.category)) {
			WikiNavNode cat;
			cat.title = a.category;
			root.children.append(cat);
			categories.insert(a.category, &root.children.last());
		}
		categories.value(a.category)->children.append(leaf);
	}
	if (!uncategorised.children.isEmpty())
		root.children.append(uncategorised);
	return root;
}

QString DirectoryBundle::renderArticleHtml(const QString& slug) const
{
	auto it = m_articles.constFind(slug);
	if (it == m_articles.constEnd())
		return {};

	QString abs = QDir(m_root).filePath(it->relFile);
	QFile f(abs);
	if (!f.open(QIODevice::ReadOnly))
		return {};
	QString body = QString::fromUtf8(f.readAll());

	// QTextDocument can render Markdown directly since Qt 5.14.
	QTextDocument doc;
	doc.setMarkdown(body);
	// Rewrite relative image paths to absolute paths so the article
	// renderer can find them.
	QString html = doc.toHtml();
	QString baseUrl =
		QUrl::fromLocalFile(QFileInfo(abs).absolutePath() + "/").toString();
	html.replace(QStringLiteral("src=\""), QStringLiteral("src=\"") + baseUrl);
	return html;
}

QStringList DirectoryBundle::searchTitles(const QString& query, int limit) const
{
	QStringList out;
	for (const auto& a : m_order) {
		if (a.title.contains(query, Qt::CaseInsensitive)) {
			out.append(a.slug);
			if (out.size() >= limit)
				break;
		}
	}
	return out;
}
