/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 */

#pragma once

#include "plugin/sdk/mmco_sdk.h"

#include <QDialog>
#include <QSplitter>
#include <QListWidget>
#include <QTextBrowser>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QString>
#include <QStringList>
#include <QVector>

struct NewsEntry {
	int feedIndex;
	QString title;
	QString link;
	QString content;
	QString author;
	QString date;
};

class NewsViewerDialog : public QDialog
{
	Q_OBJECT

  public:
	explicit NewsViewerDialog(MMCOContext* ctx, QWidget* parent = nullptr);
	~NewsViewerDialog() override = default;

	/* Populate the dialog with entries fetched via the News API.
	 * If sidebarVisible is false the sidebar starts collapsed. */
	void loadEntries(bool sidebarVisible = true);
	bool isSidebarVisible() const
	{
		return m_sidebarVisible;
	}

  private slots:
	void onEntrySelected(int row);
	void onToggleSidebar();
	void onOpenInBrowser();
	void onRefresh();

  private:
	static QString renderContent(const QString& raw);
	void setSidebarVisible(bool visible);

	MMCOContext* m_ctx = nullptr;

	QSplitter* m_splitter = nullptr;
	QWidget* m_sidebar = nullptr;
	QListWidget* m_entryList = nullptr;
	QWidget* m_contentPane = nullptr;
	QLabel* m_titleLabel = nullptr;
	QLabel* m_metaLabel = nullptr;
	QTextBrowser* m_contentView = nullptr;
	QPushButton* m_openBtn = nullptr;
	QPushButton* m_refreshBtn = nullptr;
	QPushButton* m_closeBtn = nullptr;
	QToolButton* m_toggleSideBar = nullptr;

	QVector<NewsEntry> m_entries;
	bool m_sidebarVisible = true;
};
