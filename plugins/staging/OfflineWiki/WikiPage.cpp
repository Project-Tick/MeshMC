/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "WikiPage.h"
#include "WikiBundle.h"

#include <QListWidget>
#include <QSplitter>
#include <QTextBrowser>

WikiPage::WikiPage(QList<WikiBundle*>* bundles, SearchIndex* index,
				   QWidget* parent)
	: QWidget(parent), m_bundles(bundles), m_index(index)
{
	buildUi();
	rebuildNav();
}

void WikiPage::buildUi()
{
	auto* root = new QVBoxLayout(this);

	auto* topRow = new QHBoxLayout();
	auto* addBtn = new QPushButton(tr("Add bundle…"), this);
	auto* removeBtn = new QPushButton(tr("Remove selected"), this);
	connect(addBtn, &QPushButton::clicked, this, &WikiPage::onAddBundle);
	connect(removeBtn, &QPushButton::clicked, this, &WikiPage::onRemoveBundle);
	topRow->addWidget(addBtn);
	topRow->addWidget(removeBtn);
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
	m_view->setOpenExternalLinks(false);
	split->addWidget(m_view);

	split->setStretchFactor(0, 1);
	split->setStretchFactor(1, 3);

	root->addWidget(split, /*stretch=*/1);
}

void WikiPage::rebuildNav()
{
	m_nav->clear();
	m_index->reset();
	for (auto* bundle : *m_bundles) {
		m_index->addBundle(bundle);

		auto navRoot = bundle->buildNav();
		auto* rootItem = new QTreeWidgetItem(m_nav);
		rootItem->setText(0, bundle->name());
		rootItem->setData(0, Qt::UserRole, QVariant::fromValue<void*>(bundle));

		std::function<void(QTreeWidgetItem*, const WikiNavNode&)> walk =
			[&](QTreeWidgetItem* parent, const WikiNavNode& node) {
				for (const auto& c : node.children) {
					auto* it = new QTreeWidgetItem(parent);
					it->setText(0, c.title);
					it->setData(0, Qt::UserRole,
								QVariant::fromValue<void*>(bundle));
					it->setData(0, Qt::UserRole + 1, c.slug);
					walk(it, c);
				}
			};
		walk(rootItem, navRoot);
	}
}

void WikiPage::onAddBundle()
{
	QMenu menu(this);
	auto* aDir = menu.addAction(tr("Add directory bundle…"));
	auto* aZim = menu.addAction(tr("Add ZIM file…"));
	QAction* chosen = menu.exec(QCursor::pos());

	QString path;
	if (chosen == aDir) {
		path = QFileDialog::getExistingDirectory(this, tr("Choose bundle"));
	} else if (chosen == aZim) {
		path = QFileDialog::getOpenFileName(this, tr("Choose ZIM"),
											QString(),
											tr("Kiwix ZIM (*.zim)"));
	} else {
		return;
	}
	if (path.isEmpty())
		return;
	auto* b = openBundle(path);
	if (!b) {
		QMessageBox::warning(this, tr("Offline Wiki"),
							 tr("Could not open '%1' as a bundle.").arg(path));
		return;
	}
	m_bundles->append(b);
	rebuildNav();
}

void WikiPage::onRemoveBundle()
{
	auto items = m_nav->selectedItems();
	if (items.isEmpty())
		return;
	QTreeWidgetItem* it = items.first();
	while (it->parent())
		it = it->parent();
	void* bRaw = it->data(0, Qt::UserRole).value<void*>();
	auto* b = static_cast<WikiBundle*>(bRaw);
	m_bundles->removeAll(b);
	delete b;
	rebuildNav();
}

void WikiPage::onNavSelection()
{
	auto items = m_nav->selectedItems();
	if (items.isEmpty())
		return;
	auto* it = items.first();
	void* bRaw = it->data(0, Qt::UserRole).value<void*>();
	QString slug = it->data(0, Qt::UserRole + 1).toString();
	if (!bRaw || slug.isEmpty())
		return;
	auto* b = static_cast<WikiBundle*>(bRaw);
	m_view->setHtml(b->renderArticleHtml(slug));
}

void WikiPage::onSearchTextChanged(const QString& text)
{
	m_searchResults->clear();
	if (text.trimmed().isEmpty()) {
		m_searchResults->setVisible(false);
		return;
	}
	auto hits = m_index->search(text, 200);
	for (const auto& h : hits) {
		auto* item = new QListWidgetItem(
			QStringLiteral("[%1] %2").arg(h.bundle->name(), h.title));
		item->setData(Qt::UserRole, QVariant::fromValue<void*>(h.bundle));
		item->setData(Qt::UserRole + 1, h.slug);
		m_searchResults->addItem(item);
	}
	m_searchResults->setVisible(!hits.isEmpty());
}

void WikiPage::onSearchHitChosen()
{
	auto* it = m_searchResults->currentItem();
	if (!it)
		return;
	auto* b = static_cast<WikiBundle*>(it->data(Qt::UserRole).value<void*>());
	QString slug = it->data(Qt::UserRole + 1).toString();
	if (!b)
		return;
	m_view->setHtml(b->renderArticleHtml(slug));
}
