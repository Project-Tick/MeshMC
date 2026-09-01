/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
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

#include "ui/dialogs/ExportPackDialog.h"
#include "ui_ExportPackDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

#include <functional>
#include <utility>

#include "FileIgnoreProxy.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "modplatform/flame/FlamePackExportTask.h"
#include "modplatform/modrinth/ModrinthPackExportTask.h"
#include "settings/SettingsObject.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"

namespace
{
	/* CurseForge warns about packs asking for more than this, so the
	 * figure we suggest stops here however much the instance itself is
	 * allowed to use. */
	constexpr int MAX_SUGGESTED_MEMORY_MIB = 1024 * 12;
} // namespace

ExportPackDialog::ExportPackDialog(MinecraftInstance* instance,
								   QWidget* parent, Format format)
	: QDialog(parent), ui(new Ui::ExportPackDialog), m_instance(instance),
	  m_proxyModel(nullptr), m_format(format)
{
	ui->setupUi(this);

	auto settings = m_instance->settings();

	/* The instance's name as a placeholder rather than as the value: it
	 * is what an empty field means, and prefilling it would turn a later
	 * rename into a pack that still carries the old name. */
	ui->name->setPlaceholderText(m_instance->name());
	ui->name->setText(settings->get("ExportName").toString());
	ui->version->setText(settings->get("ExportVersion").toString());
	ui->optionalFiles->setChecked(settings->get("ExportOptionalFiles").toBool());

	connect(ui->recommendedMemoryCheckBox, &QCheckBox::toggled,
			ui->recommendedMemory, &QWidget::setEnabled);

	/* The fields the other format has no use for are hidden, not
	 * disabled. A greyed-out box still asks a question, and neither
	 * catalogue has anywhere to put the answer. */
	if (m_format == Format::ModrinthPack) {
		setWindowTitle(tr("Export Modrinth Pack"));

		ui->authorLabel->hide();
		ui->author->hide();
		ui->recommendedMemoryWidget->hide();

		ui->summary->setPlainText(settings->get("ExportSummary").toString());
	} else {
		setWindowTitle(tr("Export CurseForge Pack"));

		ui->summaryLabel->hide();
		ui->summary->hide();

		ui->author->setText(settings->get("ExportAuthor").toString());

		const int recommendedRAM = settings->get("ExportRecommendedRAM").toInt();
		if (recommendedRAM > 0) {
			ui->recommendedMemoryCheckBox->setChecked(true);
			ui->recommendedMemory->setValue(recommendedRAM);
		} else {
			/* Unchecked, but with a figure already in the box: the
			 * instance's own allocation is the only informed guess
			 * available, and the user should see what they would be
			 * publishing before they agree to publish it. */
			ui->recommendedMemoryCheckBox->setChecked(false);
			ui->recommendedMemory->setValue(
				qMin(settings->get("MaxMemAlloc").toInt(),
					 MAX_SUGGESTED_MEMORY_MIB));
		}
	}

	connect(ui->name, &QLineEdit::textEdited, this,
			&ExportPackDialog::validate);
	connect(ui->version, &QLineEdit::textEdited, this,
			&ExportPackDialog::validate);
	validate();

	auto* model = new QFileSystemModel(this);
	/* The platform's own icon lookup is a per-file call, and this model
	 * is pointed at an entire game directory. */
	model->setIconProvider(&m_icons);

	/* Rooted at the instance directory even though the view starts at the
	 * game directory, so that the blocked paths this records line up with
	 * the ones the plain-zip export writes: the two share one
	 * `.packignore`, and a file unchecked in one should stay unchecked in
	 * the other. */
	const QString instanceRoot = m_instance->instanceRoot();
	m_proxyModel = new FileIgnoreProxy(instanceRoot, this);
	m_proxyModel->setSourceModel(model);

	/* Debris rather than content. Hidden instead of merely unchecked for
	 * the same reason as in the plain-zip export: there is no version of
	 * "yes, publish my crash reports" worth offering a checkbox for. */
	const QDir instanceDir(instanceRoot);
	const QString prefix = instanceDir.relativeFilePath(m_instance->gameRoot());
	for (auto path : {"logs", "crash-reports", ".cache", ".fabric", ".quilt"}) {
		m_proxyModel->ignoreFilesWithPath().insert(
			FS::PathCombine(prefix, path));
	}
	m_proxyModel->ignoreFilesWithName().append(
		{".DS_Store", "thumbs.db", "Thumbs.db"});

	/* The sidecars that record where each managed file came from. They
	 * are this launcher's bookkeeping, and in a pack they would be
	 * actively wrong: the manifest is what tells an installer where the
	 * files come from, and a stale `.pw.toml` beside a downloaded mod
	 * would claim a different origin for it. */
	m_proxyModel->ignoreFilesWithSuffix().append(".pw.toml");
	for (const QString& folder :
		 {m_instance->modsRoot(), m_instance->coreModsDir(),
		  m_instance->resourcePacksDir(), m_instance->texturePacksDir(),
		  m_instance->shaderPacksDir()}) {
		if (folder.isEmpty()) {
			continue;
		}
		/* Spelled out rather than asked of ModMetadataIndex, whose
		 * accessor creates the directory it names - which would leave an
		 * empty `.index` behind in every folder merely by opening this
		 * dialog. */
		m_proxyModel->ignoreFilesWithPath().insert(
			instanceDir.relativeFilePath(FS::PathCombine(folder, ".index")));
	}

	m_proxyModel->loadBlockedPathsFromFile(ignoreFileName());

	ui->files->setModel(m_proxyModel);
	ui->files->setRootIndex(
		m_proxyModel->mapFromSource(model->index(m_instance->gameRoot())));
	ui->files->sortByColumn(0, Qt::AscendingOrder);

	model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs |
					 QDir::Hidden);
	model->setRootPath(m_instance->gameRoot());

	auto* headerView = ui->files->header();
	headerView->setSectionResizeMode(QHeaderView::ResizeToContents);
	headerView->setSectionResizeMode(0, QHeaderView::Stretch);

	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
}

ExportPackDialog::~ExportPackDialog()
{
	delete ui;
}

void ExportPackDialog::validate()
{
	/* Only Modrinth insists: `versionId` is required in an mrpack index,
	 * where a CurseForge manifest is content with an empty version
	 * string. The name may be empty in both, because empty means "use
	 * the instance's name". */
	const bool incomplete =
		m_format == Format::ModrinthPack && ui->version->text().isEmpty();
	ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(incomplete);
}

void ExportPackDialog::saveInputs()
{
	auto settings = m_instance->settings();
	settings->set("ExportName", ui->name->text());
	settings->set("ExportVersion", ui->version->text());
	settings->set("ExportOptionalFiles", ui->optionalFiles->isChecked());

	if (m_format == Format::ModrinthPack) {
		settings->set("ExportSummary", ui->summary->toPlainText());
		return;
	}

	settings->set("ExportAuthor", ui->author->text());
	if (ui->recommendedMemoryCheckBox->isChecked()) {
		settings->set("ExportRecommendedRAM", ui->recommendedMemory->value());
	} else {
		/* Reset rather than set to zero: the setting's own default is
		 * "do not state a requirement", and storing a zero on top of it
		 * would preserve the figure the box happened to be showing. */
		settings->reset("ExportRecommendedRAM");
	}
}

QString ExportPackDialog::askForOutputPath(const QString& packName)
{
	const QString fileName = FS::RemoveInvalidFilenameChars(packName);

	/* No DontConfirmOverwrite: the dialog's own "replace this file?"
	 * prompt is the platform's, in the platform's words. */
	if (m_format == Format::ModrinthPack) {
		QString output = QFileDialog::getSaveFileName(
			this, tr("Export %1").arg(packName),
			FS::PathCombine(QDir::homePath(), fileName + ".mrpack"),
			tr("Modrinth pack") + " (*.mrpack *.zip)", nullptr);
		if (output.isEmpty()) {
			return {};
		}
		/* `.zip` is left alone: an mrpack is a zip, and a user who typed
		 * that extension meant it. Anything else gains the real one, so
		 * the file is not one the catalogue refuses to read. */
		if (!output.endsWith(".mrpack") && !output.endsWith(".zip")) {
			output.append(".mrpack");
		}
		return output;
	}

	QString output = QFileDialog::getSaveFileName(
		this, tr("Export %1").arg(packName),
		FS::PathCombine(QDir::homePath(), fileName + ".zip"),
		tr("CurseForge pack") + " (*.zip)", nullptr);
	if (output.isEmpty()) {
		return {};
	}
	if (!output.endsWith(".zip")) {
		output.append(".zip");
	}
	return output;
}

std::unique_ptr<Task>
ExportPackDialog::buildExportTask(const QString& packName,
								  const QString& output)
{
	/* Bound to the proxy rather than copied out of it: the model is what
	 * knows which entries the user unchecked, and it stays alive for as
	 * long as this dialog - which outlives the export it started. */
	auto filter = std::bind(&FileIgnoreProxy::filterFile, m_proxyModel,
							std::placeholders::_1);

	if (m_format == Format::ModrinthPack) {
		return std::make_unique<ModrinthPackExportTask>(
			packName, ui->version->text(), ui->summary->toPlainText(),
			ui->optionalFiles->isChecked(), m_instance, output,
			std::move(filter));
	}

	FlamePackExportOptions options;
	options.name = packName;
	options.version = ui->version->text();
	options.author = ui->author->text();
	options.optionalFiles = ui->optionalFiles->isChecked();
	options.instance = m_instance;
	options.output = output;
	options.filter = std::move(filter);
	options.recommendedRAM = ui->recommendedMemoryCheckBox->isChecked()
								 ? ui->recommendedMemory->value()
								 : 0;

	return std::make_unique<FlamePackExportTask>(std::move(options));
}

void ExportPackDialog::done(int result)
{
	m_proxyModel->saveBlockedPathsToFile(ignoreFileName());
	/* Saved whichever way the dialog is closing: someone who typed a
	 * summary and then thought better of exporting today should not have
	 * to type it again tomorrow. */
	saveInputs();

	if (result != QDialog::Accepted) {
		QDialog::done(result);
		return;
	}

	const QString packName = ui->name->text().isEmpty()
								 ? m_instance->name()
								 : ui->name->text();

	const QString output = askForOutputPath(packName);
	if (output.isEmpty()) {
		/* Backing out of the file dialog cancels the file dialog, not
		 * the export: this window stays up with everything still filled
		 * in. */
		return;
	}

	auto task = buildExportTask(packName, output);

	Task* const exportTask = task.get();
	connect(exportTask, &Task::failed, this,
			[this, exportTask](const QString& reason) {
				/* Stopping on request arrives here too - there is no
				 * separate signal for it - and reporting the user's own
				 * click back to them as an error is noise. */
				if (exportTask->wasAborted()) {
					return;
				}
				CustomMessageBox::selectable(this, tr("Error"), reason,
											 QMessageBox::Critical)
					->exec();
			});

	ProgressDialog progress(this);
	progress.setSkipButton(true, tr("Abort"));
	if (progress.execWithTask(std::move(task)) != QDialog::Accepted) {
		/* Failed or aborted. The window stays, so the user can adjust
		 * what they chose and try again without retyping any of it. */
		return;
	}

	QDialog::done(result);
}

QString ExportPackDialog::ignoreFileName() const
{
	return FS::PathCombine(m_instance->instanceRoot(), ".packignore");
}
