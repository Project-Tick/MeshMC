/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: Apache-2.0 */

#include "GitVersioningPage.h"
#include <KLocalizedString>

namespace
{
	QString humanSize(qint64 bytes)
	{
		if (bytes < 1024)
			return i18n("%1 B", bytes);
		double v = bytes / 1024.0;
		if (v < 1024.0)
			return i18n("%1 KiB", QString::number(v, 'f', 1));
		v /= 1024.0;
		if (v < 1024.0)
			return i18n("%1 MiB", QString::number(v, 'f', 1));
		v /= 1024.0;
		return i18n("%1 GiB", QString::number(v, 'f', 2));
	}
} // namespace

GitVersioningPage::GitVersioningPage(MMCOContext* ctx,
									 const QString& instanceId,
									 const QString& instanceRoot,
									 QWidget* parent)
	: QWidget(parent), m_ctx(ctx), m_instanceId(instanceId),
	  m_instanceRoot(instanceRoot), m_repo(instanceId, instanceRoot)
{
	buildUi();
	reloadHistory();
}

bool GitVersioningPage::confirm(const QString& title,
								const QString& message) const
{
	/* Prefer the host's confirm dialog (S-tier UI API). It returns 1
	 * on confirm, 0 on cancel. The host strips rich-text, so pass a
	 * plain-text message here. */
	if (m_ctx && m_ctx->ui_confirm_dialog) {
		return m_ctx->ui_confirm_dialog(m_ctx->module_handle, title.toUtf8(),
										message.toUtf8()) != 0;
	}
	return QMessageBox::question(const_cast<GitVersioningPage*>(this), title,
								 message) == QMessageBox::Yes;
}

QIcon GitVersioningPage::icon() const
{
	/* The launcher's icon theme is reachable through QIcon::fromTheme
	 * once Qt has been initialised; the host installs the MeshMC theme
	 * search paths during Application::init() so plugin code finds the
	 * same icon set the launcher does. */
	return QIcon::fromTheme(QStringLiteral("git-scm"));
}

void GitVersioningPage::buildUi()
{
	auto* layout = new QVBoxLayout(this);

	m_statusLabel = new QLabel(this);
	m_statusLabel->setWordWrap(true);
	layout->addWidget(m_statusLabel);

	m_tree = new QTreeWidget(this);
	m_tree->setHeaderLabels({i18n("When"), i18n("Commit"), i18n("Subject"),
							 i18n("Files"), i18n("+"), i18n("-")});
	m_tree->setRootIsDecorated(false);
	m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
	m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_tree->setAlternatingRowColors(true);
	m_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	connect(m_tree, &QTreeWidget::itemSelectionChanged, this,
			&GitVersioningPage::onSelectionChanged);
	layout->addWidget(m_tree, /*stretch=*/1);

	auto* btnRow = new QHBoxLayout();

	m_commitBtn = new QPushButton(i18n("Snapshot now"), this);
	m_commitBtn->setToolTip(i18n("Commit the current state of the instance"));
	connect(m_commitBtn, &QPushButton::clicked, this,
			&GitVersioningPage::onCommitClicked);
	btnRow->addWidget(m_commitBtn);

	m_restoreBtn = new QPushButton(i18n("Restore selected"), this);
	m_restoreBtn->setEnabled(false);
	connect(m_restoreBtn, &QPushButton::clicked, this,
			&GitVersioningPage::onRestoreClicked);
	btnRow->addWidget(m_restoreBtn);

	m_tagBtn = new QPushButton(i18n("Tag…"), this);
	m_tagBtn->setEnabled(false);
	connect(m_tagBtn, &QPushButton::clicked, this,
			&GitVersioningPage::onTagClicked);
	btnRow->addWidget(m_tagBtn);

	m_dropBtn = new QPushButton(i18n("Drop last commit"), this);
	m_dropBtn->setToolTip(i18n("Hard-reset HEAD by one commit. The dropped "
							 "commit is unreachable but reachable again via "
							 "the reflog for ~30 days."));
	connect(m_dropBtn, &QPushButton::clicked, this,
			&GitVersioningPage::onDropClicked);
	btnRow->addWidget(m_dropBtn);

	btnRow->addStretch();

	m_refreshBtn = new QPushButton(i18n("Refresh"), this);
	connect(m_refreshBtn, &QPushButton::clicked, this,
			&GitVersioningPage::onRefresh);
	btnRow->addWidget(m_refreshBtn);

	layout->addLayout(btnRow);
}

void GitVersioningPage::reloadHistory()
{
	auto st = m_repo.status();
	if (!GitRepo::gitAvailable()) {
		m_statusLabel->setText(i18n("<b>git</b> is not installed on your system. "
								  "Install Git and reopen this page."));
		m_commitBtn->setEnabled(false);
		m_restoreBtn->setEnabled(false);
		m_tagBtn->setEnabled(false);
		m_dropBtn->setEnabled(false);
		return;
	}

	if (!st.initialized) {
		m_statusLabel->setText(
			i18n("No version history yet. Snapshot now to start tracking "
			   "changes to this instance."));
	} else {
		QStringList parts;
		parts << i18n("HEAD: <b>%1</b>", st.head);
		if (st.dirty) {
			parts << i18n("Modified: %1, Deleted: %2, Untracked: %3", st.modifiedCount, st.deletedCount, st.untrackedCount);
		} else {
			parts << i18n("Clean working tree");
		}
		parts << i18n("git %1", GitRepo::gitVersion());
		m_statusLabel->setText(parts.join(QStringLiteral(" — ")));
	}

	m_commits = m_repo.log();
	m_tree->clear();
	// Match the BackupSystem plugin's fixed timestamp format — sortable
	// and unambiguous across locales. Qt6 removed
	// Qt::DefaultLocaleShortDate so an explicit format string is the
	// most portable choice anyway.
	for (auto& c : m_commits) {
		m_repo.fillCommitStats(c);
		auto* item = new QTreeWidgetItem(m_tree);
		item->setText(0,
					  c.when.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
		item->setText(1, c.sha);
		QString subject = c.subject;
		if (c.isPreLaunch)
			subject = QStringLiteral("⚡ ") + subject;
		item->setText(2, subject);
		item->setText(3, QString::number(c.filesChanged));
		item->setText(4, humanSize(c.sizeAdded));
		item->setText(5, humanSize(c.sizeRemoved));
		item->setData(0, Qt::UserRole, c.fullSha);
	}

	onSelectionChanged();
}

GitCommit GitVersioningPage::selectedCommit() const
{
	auto items = m_tree->selectedItems();
	if (items.isEmpty())
		return {};
	QString sha = items.first()->data(0, Qt::UserRole).toString();
	for (const auto& c : m_commits)
		if (c.fullSha == sha)
			return c;
	return {};
}

void GitVersioningPage::onSelectionChanged()
{
	bool has = !m_tree->selectedItems().isEmpty();
	m_restoreBtn->setEnabled(has);
	m_tagBtn->setEnabled(has);
}

void GitVersioningPage::onCommitClicked()
{
	bool ok = false;
	QString msg = QInputDialog::getText(
		this, i18n("Snapshot"),
		i18n("Describe the changes you're snapshotting (leave blank for an "
		   "auto-generated message):"),
		QLineEdit::Normal,
		i18n("Manual snapshot %1", QDateTime::currentDateTime().toString( QStringLiteral("yyyy-MM-dd HH:mm"))),
		&ok);
	if (!ok)
		return;
	QString err;
	QString sha = m_repo.commit(msg, /*isPreLaunch=*/false, &err);
	if (sha.isEmpty() && !err.isEmpty()) {
		QMessageBox::warning(this, i18n("Snapshot failed"), err);
	} else if (sha.isEmpty()) {
		QMessageBox::information(
			this, i18n("Snapshot"),
			i18n("Nothing to commit — the working tree was clean."));
	}
	reloadHistory();
}

void GitVersioningPage::onRestoreClicked()
{
	auto c = selectedCommit();
	if (c.fullSha.isEmpty())
		return;
	if (!confirm(i18n("Restore?"),
				 i18n("Restore the instance to commit %1 (%2)?\n\n"
					"An auto-snapshot of the current state is taken first, "
					"so this can be undone by restoring the previous "
					"snapshot.", c.sha, c.subject)))
		return;
	QString err;
	if (!m_repo.restore(c.fullSha, &err))
		QMessageBox::warning(this, i18n("Restore failed"), err);
	reloadHistory();
}

void GitVersioningPage::onTagClicked()
{
	auto c = selectedCommit();
	if (c.fullSha.isEmpty())
		return;
	bool ok = false;
	QString name = QInputDialog::getText(
		this, i18n("Tag commit"), i18n("Tag name:"), QLineEdit::Normal,
		QStringLiteral("milestone-%1")
			.arg(c.when.toString(QStringLiteral("yyyyMMdd"))),
		&ok);
	if (!ok || name.isEmpty())
		return;
	QString err;
	if (!m_repo.tag(name, c.fullSha, &err))
		QMessageBox::warning(this, i18n("Tag failed"), err);
}

void GitVersioningPage::onDropClicked()
{
	if (!confirm(i18n("Drop last commit?"),
				 i18n("This hard-resets HEAD by one commit. The working "
					"tree is reset to the parent commit too. Continue?")))
		return;
	QString err;
	if (!m_repo.dropHead(&err))
		QMessageBox::warning(this, i18n("Drop failed"), err);
	reloadHistory();
}

void GitVersioningPage::onRefresh()
{
	reloadHistory();
}
