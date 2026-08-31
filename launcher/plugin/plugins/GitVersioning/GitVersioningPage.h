/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * GitVersioningPage — instance page that exposes the per-instance
 * commit history as a BasePage subclass.
 */

#pragma once

#include "plugin/sdk/mmco_cxx_sdk.hpp"
#include "GitRepo.h"

class GitVersioningPage : public QWidget, public BasePage
{
	Q_OBJECT
  public:
	/* Constructed from a string instance id + filesystem root rather
	 * than InstancePtr — keeps the page off the launcher type system.
	 * `ctx` is the MMCO context so destructive operations can route
	 * their confirmation prompts through the host's ui_confirm_dialog
	 * (S-tier UI API) rather than a plugin-local QMessageBox. It may
	 * be null, in which case the page falls back to QMessageBox. */
	GitVersioningPage(MMCOContext* ctx, const QString& instanceId,
					  const QString& instanceRoot, QWidget* parent = nullptr);

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

	/* Destructive-action confirmation. Routes through the host's
	 * ui_confirm_dialog when a context is available so the prompt is
	 * styled and themed like the rest of the launcher; otherwise it
	 * falls back to a plugin-local QMessageBox. Returns true when the
	 * user confirms. */
	bool confirm(const QString& title, const QString& message) const;

	MMCOContext* m_ctx = nullptr;
	QString m_instanceId;
	QString m_instanceRoot;
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
