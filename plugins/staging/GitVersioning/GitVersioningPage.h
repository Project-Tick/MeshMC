/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * GitVersioningPage — instance page that exposes the per-instance
 * commit history as a BasePage subclass.
 */

#pragma once

#include "plugin/sdk/mmco_sdk.h"
#include "GitRepo.h"

class GitVersioningPage : public QWidget, public BasePage
{
	Q_OBJECT
  public:
	explicit GitVersioningPage(InstancePtr instance, QWidget* parent = nullptr);

	QString id() const override
	{
		return QStringLiteral("git-versioning");
	}
	QString displayName() const override
	{
		return QObject::tr("Version History");
	}
	QIcon icon() const override;
	QString helpPage() const override
	{
		return QStringLiteral("Git-Versioning");
	}
	bool shouldDisplay() const override
	{
		return true;
	}

  private slots:
	void onCommitClicked();
	void onRestoreClicked();
	void onTagClicked();
	void onDropClicked();
	void onRefresh();
	void onSelectionChanged();

  private:
	void buildUi();
	void reloadHistory();
	GitCommit selectedCommit() const;

	InstancePtr m_instance;
	GitRepo m_repo;
	QList<GitCommit> m_commits;

	QLabel* m_statusLabel = nullptr;
	QTreeWidget* m_tree = nullptr;
	QPushButton* m_commitBtn = nullptr;
	QPushButton* m_restoreBtn = nullptr;
	QPushButton* m_tagBtn = nullptr;
	QPushButton* m_dropBtn = nullptr;
	QPushButton* m_refreshBtn = nullptr;
};
