/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
 * Copyright (C) 2026 Project Tick
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ExportInstanceDialog.h"
#include "ui_ExportInstanceDialog.h"

#include <BaseInstance.h>
#include <FileSystem.h>
#include <MMCZip.h>
#include <icons/IconList.h>

#include <QDebug>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>

#include <functional>
#include <memory>

#include "Application.h"
#include "FileIgnoreProxy.h"
#include "archive/ExportToZipTask.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"

ExportInstanceDialog::ExportInstanceDialog(InstancePtr instance,
										   QWidget* parent)
	: QDialog(parent), ui(new Ui::ExportInstanceDialog), m_instance(instance)
{
	ui->setupUi(this);

	auto model = new QFileSystemModel(this);
	/* The platform's own icon lookup is a per-file call, and this model
	 * is pointed at an entire instance directory. */
	model->setIconProvider(&m_icons);

	const QString root = instance->instanceRoot();
	m_proxyModel = new FileIgnoreProxy(root, this);
	m_proxyModel->setSourceModel(model);

	/* Things that are not part of an instance so much as debris it
	 * produces. They are hidden rather than merely unchecked: an export
	 * that carries somebody else's crash reports and caches is bigger
	 * and less useful, and there is no version of "yes, please include
	 * my logs" worth offering a checkbox for. */
	const QString prefix = QDir(root).relativeFilePath(instance->gameRoot());
	for (auto path : {"logs", "crash-reports", ".cache", ".fabric", ".quilt"}) {
		m_proxyModel->ignoreFilesWithPath().insert(
			FS::PathCombine(prefix, path));
	}
	m_proxyModel->ignoreFilesWithName().append(
		{".DS_Store", "thumbs.db", "Thumbs.db"});
	m_proxyModel->loadBlockedPathsFromFile(ignoreFileName());

	ui->treeView->setModel(m_proxyModel);
	ui->treeView->setRootIndex(m_proxyModel->mapFromSource(model->index(root)));
	ui->treeView->sortByColumn(0, Qt::AscendingOrder);

	connect(m_proxyModel, &QAbstractItemModel::rowsInserted, this,
			&ExportInstanceDialog::rowsInserted);

	model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs |
					 QDir::Hidden);
	model->setRootPath(root);

	auto headerView = ui->treeView->header();
	headerView->setSectionResizeMode(QHeaderView::ResizeToContents);
	headerView->setSectionResizeMode(0, QHeaderView::Stretch);
}

ExportInstanceDialog::~ExportInstanceDialog()
{
	delete ui;
}

/// Save icon to instance's folder is needed
void SaveIcon(InstancePtr m_instance)
{
	auto iconKey = m_instance->iconKey();
	auto iconList = APPLICATION->icons();
	auto mmcIcon = iconList->icon(iconKey);
	if (!mmcIcon || mmcIcon->isBuiltIn()) {
		return;
	}
	auto path = mmcIcon->getFilePath();
	if (!path.isNull()) {
		QFileInfo inInfo(path);
		FS::copy(path, FS::PathCombine(m_instance->instanceRoot(),
									   inInfo.fileName()))();
		return;
	}
	auto& image = mmcIcon->m_images[mmcIcon->type()];
	auto& icon = image.icon;
	auto sizes = icon.availableSizes();
	if (sizes.size() == 0) {
		return;
	}
	auto areaOf = [](QSize size) { return size.width() * size.height(); };
	QSize largest = sizes[0];
	// find variant with largest area
	for (auto size : sizes) {
		if (areaOf(largest) < areaOf(size)) {
			largest = size;
		}
	}
	auto pixmap = icon.pixmap(largest);
	pixmap.save(FS::PathCombine(m_instance->instanceRoot(), iconKey + ".png"));
}

void ExportInstanceDialog::doExport()
{
	const auto name = FS::RemoveInvalidFilenameChars(m_instance->name());

	const QString output = QFileDialog::getSaveFileName(
		this, tr("Export %1").arg(m_instance->name()),
		FS::PathCombine(QDir::homePath(), name + ".zip"), "Zip (*.zip)",
		nullptr);
	/* No DontConfirmOverwrite: the dialog's own "replace this file?"
	 * prompt is the platform's, in the platform's words, and asking the
	 * same question again afterwards in our own only made it look like
	 * two different questions. */
	if (output.isEmpty()) {
		QDialog::done(QDialog::Rejected);
		return;
	}

	SaveIcon(m_instance);

	QFileInfoList files;
	if (!MMCZip::collectFileListRecursively(
			m_instance->instanceRoot(), QString(), &files,
			std::bind(&FileIgnoreProxy::filterFile, m_proxyModel,
					  std::placeholders::_1))) {
		QMessageBox::warning(this, tr("Error"),
							 tr("Unable to export instance"));
		QDialog::done(QDialog::Rejected);
		return;
	}

	auto task = std::make_unique<MMCZip::ExportToZipTask>(
		output, m_instance->instanceRoot(), files, QString(), true);

	/* Shown with exec() rather than show(): this dialog closes the
	 * moment the progress dialog returns, and a message box parented to
	 * a dialog that is going away is a box the user never gets to
	 * read. */
	Task* const exportTask = task.get();
	connect(exportTask, &Task::failed, this,
			[this, exportTask](const QString& reason) {
				/* Stopping on request arrives here too - there is no
				 * separate signal for it - and reporting the user's own
				 * click back to them as an error is not a report, it is
				 * noise. */
				if (exportTask->wasAborted()) {
					return;
				}
				CustomMessageBox::selectable(this, tr("Error"), reason,
											 QMessageBox::Critical)
					->exec();
			});

	ProgressDialog progress(this);
	progress.setSkipButton(true, tr("Abort"));
	QDialog::done(progress.execWithTask(std::move(task)));
}

void ExportInstanceDialog::done(int result)
{
	m_proxyModel->saveBlockedPathsToFile(ignoreFileName());
	if (result == QDialog::Accepted) {
		doExport();
		return;
	}
	QDialog::done(result);
}

void ExportInstanceDialog::rowsInserted(QModelIndex parent, int top,
										int bottom)
{
	// WARNING: possible off-by-one?
	for (int i = top; i < bottom; i++) {
		auto node = m_proxyModel->index(i, 0, parent);
		if (m_proxyModel->shouldExpand(node)) {
			auto expNode = node.parent();
			if (!expNode.isValid()) {
				continue;
			}
			ui->treeView->expand(node);
		}
	}
}

QString ExportInstanceDialog::ignoreFileName()
{
	return FS::PathCombine(m_instance->instanceRoot(), ".packignore");
}
