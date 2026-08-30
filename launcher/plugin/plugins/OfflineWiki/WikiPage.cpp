/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "WikiPage.h"
#include "WikiRepoBundle.h"

#include <QDesktopServices>
#include <QListWidget>
#include <QScrollBar>
#include <QSplitter>
#include <QTextBrowser>
#include <QUrl>

WikiPage::WikiPage(WikiRepoBundle** bundle, bool gitAvailable, QWidget* parent)
	: QWidget(parent), m_bundle(bundle), m_gitAvailable(gitAvailable)
{
	buildUi();
	rebuildNav();
}

void WikiPage::buildUi()
{
	auto* root = new QVBoxLayout(this);

	auto* topRow = new QHBoxLayout();
	topRow->addStretch();
	m_searchEdit = new QLineEdit(this);
	m_searchEdit->setPlaceholderText(tr("Search…"));
	topRow->addWidget(m_searchEdit, /*stretch=*/2);
	connect(m_searchEdit, &QLineEdit::textChanged, this,
			&WikiPage::onSearchTextChanged);
	root->addLayout(topRow);

	auto* split = new QSplitter(Qt::Horizontal, this);

	auto* leftCol = new QWidget(split);
	auto* lv = new QVBoxLayout(leftCol);
	lv->setContentsMargins(0, 0, 0, 0);
	m_nav = new QTreeWidget(leftCol);
	m_nav->setHeaderHidden(true);
	connect(m_nav, &QTreeWidget::itemSelectionChanged, this,
			&WikiPage::onNavSelection);
	lv->addWidget(m_nav, 2);
	m_searchResults = new QListWidget(leftCol);
	m_searchResults->setVisible(false);
	connect(m_searchResults, &QListWidget::itemActivated, this,
			[this](QListWidgetItem*) { onSearchHitChosen(); });
	connect(m_searchResults, &QListWidget::itemSelectionChanged, this,
			&WikiPage::onSearchHitChosen);
	lv->addWidget(m_searchResults, 1);

	split->addWidget(leftCol);

	m_view = new QTextBrowser(split);
	// We handle every link ourselves: internal "wiki:" links navigate
	// between articles, external links open in the system browser. So
	// disable QTextBrowser's own navigation and route anchorClicked.
	m_view->setOpenLinks(false);
	m_view->setOpenExternalLinks(false);
	connect(m_view, &QTextBrowser::anchorClicked, this,
			&WikiPage::onAnchorClicked);
	split->addWidget(m_view);

	split->setStretchFactor(0, 1);
	split->setStretchFactor(1, 3);

	root->addWidget(split, /*stretch=*/1);
}

void WikiPage::refreshBundle()
{
	rebuildNav();
}

void WikiPage::rebuildNav()
{
	m_nav->clear();

	WikiRepoBundle* b = bundle();
	if (b && b->isOpen()) {
		auto* rootItem = new QTreeWidgetItem(m_nav);
		rootItem->setText(0, b->name());
		rootItem->setExpanded(true);
		for (const auto& e : b->nav()) {
			auto* it = new QTreeWidgetItem(rootItem);
			it->setText(0, e.title);
			it->setData(0, Qt::UserRole, e.slug);
		}
		return;
	}

	// No wiki yet: explain why rather than leaving a blank viewer.
	if (!m_gitAvailable) {
		m_view->setHtml(
			tr("<h3>Wiki not available</h3>"
			   "<p>The MeshMC wiki is downloaded automatically using "
			   "<b>git</b>, but no <code>git</code> program was found on your "
			   "system, so the wiki cannot be fetched.</p>"
			   "<p>Install Git and restart MeshMC to download the wiki.</p>"));
	} else {
		m_view->setHtml(
			tr("<h3>Downloading the MeshMC wiki…</h3>"
			   "<p>The wiki is fetched in the background the first time you "
			   "are online and will appear here automatically once the "
			   "download finishes. If you are offline, MeshMC retries on "
			   "every launch.</p>"));
	}
}

void WikiPage::onNavSelection()
{
	auto items = m_nav->selectedItems();
	if (items.isEmpty())
		return;
	const QString slug = items.first()->data(0, Qt::UserRole).toString();
	if (slug.isEmpty())
		return;
	showArticle(slug);
}

void WikiPage::showArticle(const QString& slug)
{
	WikiRepoBundle* b = bundle();
	if (!b || slug.isEmpty())
		return;
	const QString html = b->renderArticleHtml(slug);
	if (html.isEmpty()) {
		m_view->setHtml(tr("<p><i>Article not found: %1</i></p>")
							.arg(slug.toHtmlEscaped()));
		return;
	}
	m_view->setHtml(html);
	m_view->verticalScrollBar()->setValue(0);
}

void WikiPage::onAnchorClicked(const QUrl& url)
{
	// Internal wiki link (wiki:Page-Name[#fragment]) → render the target.
	if (url.scheme() == QStringLiteral("wiki")) {
		QString slug = url.path();
		if (slug.isEmpty())
			slug = url.toString().mid(QStringLiteral("wiki:").size());
		int hash = slug.indexOf(QLatin1Char('#'));
		if (hash >= 0)
			slug = slug.left(hash);
		showArticle(slug);
		return;
	}

	// Pure in-page anchor (#section) → let the browser scroll to it.
	if (url.scheme().isEmpty() && !url.fragment().isEmpty()) {
		m_view->scrollToAnchor(url.fragment());
		return;
	}

	// Everything else (http/https/mailto/…) opens in the system browser;
	// we never load remote content inside the offline viewer.
	if (!url.scheme().isEmpty())
		QDesktopServices::openUrl(url);
}

void WikiPage::onSearchTextChanged(const QString& text)
{
	m_searchResults->clear();
	WikiRepoBundle* b = bundle();
	if (!b || text.trimmed().isEmpty()) {
		m_searchResults->setVisible(false);
		return;
	}
	const auto hits = b->searchTitles(text, 200);
	for (const auto& h : hits) {
		auto* item = new QListWidgetItem(h.title);
		item->setData(Qt::UserRole, h.slug);
		m_searchResults->addItem(item);
	}
	m_searchResults->setVisible(!hits.isEmpty());
}

void WikiPage::onSearchHitChosen()
{
	auto* it = m_searchResults->currentItem();
	if (!it)
		return;
	const QString slug = it->data(Qt::UserRole).toString();
	if (slug.isEmpty())
		return;
	showArticle(slug);
}
