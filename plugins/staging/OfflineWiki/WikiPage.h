/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * WikiPage — global settings page. Two columns:
 *   • left: tree of bundles → categories → articles
 *   • right: search bar + article viewer
 */

#pragma once

#include "plugin/sdk/mmco_sdk.h"
#include "SearchIndex.h"

class QTextBrowser;
class QListWidget;
class WikiBundle;

class WikiPage : public QWidget, public BasePage
{
	Q_OBJECT
  public:
	WikiPage(QList<WikiBundle*>* bundles, SearchIndex* index,
			 QWidget* parent = nullptr);

	QString id() const override
	{
		return QStringLiteral("offline-wiki");
	}
	QString displayName() const override
	{
		return QObject::tr("Offline Wiki");
	}
	QIcon icon() const override
	{
		return QIcon::fromTheme(QStringLiteral("help-browser"));
	}

  private slots:
	void onAddBundle();
	void onRemoveBundle();
	void onNavSelection();
	void onSearchTextChanged(const QString& text);
	void onSearchHitChosen();

  private:
	void buildUi();
	void rebuildNav();

	QList<WikiBundle*>* m_bundles = nullptr;
	SearchIndex* m_index = nullptr;
	QTreeWidget* m_nav = nullptr;
	QLineEdit* m_searchEdit = nullptr;
	QListWidget* m_searchResults = nullptr;
	QTextBrowser* m_view = nullptr;
};
