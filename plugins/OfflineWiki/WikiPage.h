/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * WikiPage — global settings page for the MeshMC offline wiki. Two
 * columns:
 *   • left:  page list (+ search box)
 *   • right: article viewer
 *
 * The wiki itself (a single WikiRepoBundle) is owned by the plugin and
 * may be null until the background clone finishes; the page renders an
 * informative empty state in that case.
 */

#pragma once

#include "plugin/sdk/mmco_sdk.h"

class QTextBrowser;
class QListWidget;
class WikiRepoBundle;

class WikiPage : public QWidget, public BasePage
{
	Q_OBJECT
  public:
	/* `bundle` is a pointer-to-pointer owned by the plugin: *bundle is
	 * null until the MeshMC wiki has been cloned, and changes when a
	 * background clone completes. `gitAvailable` tells the page whether
	 * the host has a usable `git` binary so the empty state can explain
	 * why the wiki is (not yet) present. */
	WikiPage(WikiRepoBundle** bundle, bool gitAvailable,
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

	/* Re-read the wiki into the nav. Called by the plugin when a
	 * background clone/update changes what is available. */
	void refreshBundle();

  private slots:
	void onNavSelection();
	void onSearchTextChanged(const QString& text);
	void onSearchHitChosen();
	void onAnchorClicked(const QUrl& url);

  private:
	void buildUi();
	void rebuildNav();
	void showArticle(const QString& slug);
	WikiRepoBundle* bundle() const
	{
		return m_bundle ? *m_bundle : nullptr;
	}

	WikiRepoBundle** m_bundle = nullptr; // owned by the plugin
	bool m_gitAvailable = true;
	QTreeWidget* m_nav = nullptr;
	QLineEdit* m_searchEdit = nullptr;
	QListWidget* m_searchResults = nullptr;
	QTextBrowser* m_view = nullptr;
};
