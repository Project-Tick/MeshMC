/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 */

#include "NewsViewerDialog.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QSizePolicy>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>

#include <cmark.h>

#include "news/NewsChecker.h"

namespace
{
	bool looksLikeHtml(const QString& s)
	{
		static const QRegularExpression tag(QStringLiteral("<[a-zA-Z][^>]*>"));
		return s.contains(tag);
	}
} // namespace

NewsViewerDialog::NewsViewerDialog(NewsChecker* checker, QWidget* parent)
	: QDialog(parent), m_checker(checker)
{
	setWindowTitle(tr("News"));
	setSizeGripEnabled(true);
	resize(800, 500);

	/* ── Article list (left) ── */
	m_articleList = new QListWidget(this);
	m_articleList->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

	auto* leftLayout = new QVBoxLayout();
	leftLayout->addWidget(m_articleList);

	/* ── Article (right) ── */
	m_articleTitleLabel = new QLabel(this);
	m_articleTitleLabel->setAlignment(Qt::AlignCenter);
	m_articleTitleLabel->setOpenExternalLinks(true);
	m_articleTitleLabel->setWordWrap(true);

	m_articleContent = new QTextBrowser(this);
	m_articleContent->setOpenExternalLinks(true);
	m_articleContent->setOpenLinks(true);
	m_articleContent->setTextInteractionFlags(
		Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse |
		Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard |
		Qt::TextSelectableByMouse);

	auto* rightLayout = new QVBoxLayout();
	rightLayout->addWidget(m_articleTitleLabel);
	rightLayout->addWidget(m_articleContent);

	auto* splitLayout = new QHBoxLayout();
	splitLayout->addLayout(leftLayout);
	splitLayout->addLayout(rightLayout);

	/* ── Bottom row ── */
	m_toggleListButton = new QPushButton(tr("Hide article list"), this);

	m_closeButton = new QPushButton(tr("Close"), this);
	QSizePolicy closePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	closePolicy.setHorizontalStretch(10);
	m_closeButton->setSizePolicy(closePolicy);

	auto* buttonLayout = new QGridLayout();
	buttonLayout->addWidget(m_toggleListButton, 0, 0);
	buttonLayout->addWidget(m_closeButton, 0, 1);

	auto* root = new QVBoxLayout(this);
	root->addLayout(splitLayout);
	root->addLayout(buttonLayout);

	/* ── Connections ── */
	connect(m_articleList, &QListWidget::currentRowChanged, this,
			&NewsViewerDialog::selectedArticleChanged);
	connect(m_toggleListButton, &QPushButton::clicked, this,
			&NewsViewerDialog::toggleArticleList);
	connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);

	/* A reload started from anywhere — the main window, a plugin
	 * through news_reload() — repopulates us when it lands. The plugin
	 * this replaces guessed with a 2.5 second timer; being in core we
	 * can just listen to the checker. */
	if (m_checker) {
		connect(m_checker, &NewsChecker::newsLoaded, this,
				&NewsViewerDialog::onNewsLoaded);
		connect(m_checker, &NewsChecker::newsLoadingFailed, this,
				[this](const QString&) { onNewsLoaded(); });
	}
}

void NewsViewerDialog::setArticleListHidden(bool hidden)
{
	m_articleListHidden = hidden;
	m_articleList->setHidden(hidden);
	m_toggleListButton->setText(hidden ? tr("Show article list")
									   : tr("Hide article list"));
}

void NewsViewerDialog::showPlaceholder(const QString& message)
{
	m_articleTitleLabel->setText(message);
	m_articleContent->clear();
}

QString NewsViewerDialog::feedLabel(int feedIndex) const
{
	if (feedIndex == 0)
		return tr("Official");

	if (m_checker) {
		const QStringList urls = m_checker->feedUrls();
		if (feedIndex > 0 && feedIndex < urls.size()) {
			const QString host = QUrl(urls.at(feedIndex)).host();
			if (!host.isEmpty())
				return host;
		}
	}
	return tr("Feed %1").arg(feedIndex);
}

void NewsViewerDialog::loadEntries(bool listVisible)
{
	m_entries.clear();
	m_articleList->clear();

	setArticleListHidden(!listVisible);

	if (!m_checker) {
		showPlaceholder(tr("No news available."));
		return;
	}

	m_entries = m_checker->getNewsEntries();
	if (m_entries.isEmpty()) {
		showPlaceholder(m_checker->isLoadingNews() ? tr("Loading news...")
												   : tr("No news available."));
		return;
	}

	for (const auto& entry : m_entries) {
		auto* item = new QListWidgetItem(m_articleList);
		item->setText(entry->title.isEmpty() ? tr("(no title)")
											 : entry->title);
		/* Which feed an entry came from is only interesting when there
		 * is more than one, and it has no room in the list itself. */
		item->setToolTip(QStringLiteral("[%1] %2")
							 .arg(feedLabel(entry->feedIndex),
								  entry->pubDate.toString(Qt::ISODate)));
		m_articleList->addItem(item);
	}

	/* Always start on the first (latest) entry. */
	m_articleList->setCurrentRow(0);
}

void NewsViewerDialog::onNewsLoaded()
{
	loadEntries(!m_articleListHidden);
}

void NewsViewerDialog::selectedArticleChanged(int row)
{
	if (row < 0 || row >= m_entries.size())
		return;

	const NewsEntryPtr& entry = m_entries[row];
	const QString title =
		entry->title.isEmpty() ? tr("(no title)") : entry->title;

	if (entry->link.isEmpty()) {
		m_articleTitleLabel->setText(title.toHtmlEscaped());
	} else {
		m_articleTitleLabel->setText(QStringLiteral("<a href='%1'>%2</a>")
										 .arg(entry->link.toHtmlEscaped(),
											  title.toHtmlEscaped()));
	}

	m_articleContent->setHtml(renderContent(entry->content));
}

void NewsViewerDialog::toggleArticleList()
{
	setArticleListHidden(!m_articleListHidden);
}

/* static */
QString NewsViewerDialog::renderContent(const QString& raw)
{
	if (raw.isEmpty())
		return QStringLiteral("<p><i>No content.</i></p>");

	if (looksLikeHtml(raw)) {
		return QStringLiteral("<html><body style='font-family:sans-serif;'>") +
			   raw + QStringLiteral("</body></html>");
	}

	QByteArray utf8 = raw.toUtf8();
	char* html = cmark_markdown_to_html(
		utf8.constData(), static_cast<size_t>(utf8.size()), CMARK_OPT_DEFAULT);
	QString result;
	if (html) {
		result =
			QStringLiteral("<html><body style='font-family:sans-serif;'>") +
			QString::fromUtf8(html) + QStringLiteral("</body></html>");
		free(html);
	} else {
		result = QStringLiteral("<pre>") + raw.toHtmlEscaped() +
				 QStringLiteral("</pre>");
	}
	return result;
}
