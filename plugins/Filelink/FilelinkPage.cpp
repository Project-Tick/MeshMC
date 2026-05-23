/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "FilelinkPage.h"

FilelinkPage::FilelinkPage(const QString& instanceId,
						   const QString& instanceRoot, FilelinkManager* mgr,
						   MMCOContext* ctx)
	: m_instanceId(instanceId), m_instanceRoot(instanceRoot), m_manager(mgr),
	  m_ctx(ctx)
{
	ui.setupUi(this);
	refreshTable();
}

QIcon FilelinkPage::icon() const
{
	return QIcon::fromTheme("emblem-symbolic-link",
							QIcon(":/icons/toolbar/link"));
}

void FilelinkPage::refreshTable()
{
	ui.linkTreeWidget->clear();

	/* Helper: resolve an instance id to its display name via the C ABI.
	 * Empty / unknown ids fall back to the raw id string. */
	auto nameFor = [this](const QString& id) -> QString {
		if (!m_ctx || id.isEmpty())
			return id;
		const char* n = m_ctx->instance_get_name(m_ctx->module_handle,
												 id.toUtf8().constData());
		return n ? QString::fromUtf8(n) : id;
	};

	auto links = m_manager->linksForInstance(m_instanceId);
	for (const auto& e : links) {
		auto* item = new QTreeWidgetItem(ui.linkTreeWidget);
		item->setText(0, e.fileName);
		item->setText(1, e.subDir);

		// Show direction relative to this instance
		if (e.targetInstance == m_instanceId) {
			item->setText(2, tr("From: %1").arg(nameFor(e.sourceInstance)));
		} else {
			item->setText(2, tr("To: %1").arg(nameFor(e.targetInstance)));
		}

		item->setText(3, QLocale().toString(e.linkedAt, QLocale::ShortFormat));
		item->setData(0, Qt::UserRole, e.targetPath);
	}

	ui.statusLabel->setText(tr("%n linked file(s)", "", links.size()));
}

void FilelinkPage::on_linkButton_clicked()
{
	if (!m_ctx)
		return;

	/* Build a list of other instances via the C ABI. */
	QStringList names;
	QStringList ids;
	const int total = m_ctx->instance_count(m_ctx->module_handle);
	for (int i = 0; i < total; i++) {
		const char* idC = m_ctx->instance_get_id(m_ctx->module_handle, i);
		if (!idC)
			continue;
		const QString id = QString::fromUtf8(idC);
		if (id == m_instanceId)
			continue;
		const char* nameC = m_ctx->instance_get_name(m_ctx->module_handle, idC);
		names << (nameC ? QString::fromUtf8(nameC) : id);
		ids << id;
	}

	if (names.isEmpty()) {
		QMessageBox::information(
			this, tr("Filelink"),
			tr("No other instances available to link from."));
		return;
	}

	bool ok = false;
	QString chosen = QInputDialog::getItem(this, tr("Link from instance"),
										   tr("Select the source instance:"),
										   names, 0, false, &ok);
	if (!ok || chosen.isEmpty())
		return;

	int idx = names.indexOf(chosen);
	if (idx < 0)
		return;

	QString sourceId = ids[idx];
	/* The source instance's filesystem root via the C ABI. */
	const char* sourceRootC = m_ctx->instance_get_path(
		m_ctx->module_handle, sourceId.toUtf8().constData());
	if (!sourceRootC)
		return;
	const QString sourceRoot = QString::fromUtf8(sourceRootC);

	// Pick which sub-directory to link
	QStringList subDirs = {"mods", "resourcepacks", "shaderpacks"};
	QString subDir = QInputDialog::getItem(this, tr("Select folder to link"),
										   tr("Which folder should be linked?"),
										   subDirs, 0, false, &ok);
	if (!ok || subDir.isEmpty())
		return;

	QString srcDir = QDir(sourceRoot).filePath(".minecraft/" + subDir);
	QString dstDir = QDir(m_instanceRoot).filePath(".minecraft/" + subDir);

	int count = m_manager->linkDirectory(sourceId, srcDir, m_instanceId, dstDir,
										 subDir);

	if (count < 0) {
		QMessageBox::warning(this, tr("Filelink"),
							 tr("Failed to link files. The source folder may "
								"not exist."));
	} else {
		QMessageBox::information(
			this, tr("Filelink"),
			tr("Successfully linked %n file(s).", "", count));
	}

	refreshTable();
}

void FilelinkPage::on_unlinkButton_clicked()
{
	auto* item = ui.linkTreeWidget->currentItem();
	if (!item)
		return;

	QString targetPath = item->data(0, Qt::UserRole).toString();
	if (targetPath.isEmpty())
		return;

	auto answer = QMessageBox::question(
		this, tr("Unlink file"),
		tr("Remove the link for '%1'?\n\nThe linked file will be deleted.")
			.arg(item->text(0)));

	if (answer != QMessageBox::Yes)
		return;

	m_manager->unlinkFile(m_instanceId, targetPath);
	refreshTable();
}

void FilelinkPage::on_verifyButton_clicked()
{
	auto broken = m_manager->verifyLinks(m_instanceId);
	if (broken.isEmpty()) {
		QMessageBox::information(this, tr("Filelink"),
								 tr("All links are intact."));
	} else {
		QMessageBox::warning(
			this, tr("Filelink"),
			tr("%n broken link(s) found:\n\n%1", "", broken.size())
				.arg(broken.join("\n")));
	}
}
