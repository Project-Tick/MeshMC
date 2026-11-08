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

#include "ExternalToolsPage.h"
#include "ui_ExternalToolsPage.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTabBar>

#include "settings/SettingsObject.h"
#include "tools/BaseProfiler.h"
#include <FileSystem.h>
#include "Application.h"
#include "settings/GlobalSettings.h"
#include <tools/MCEditTool.h>
#include <KLocalizedString>

ExternalToolsPage::ExternalToolsPage(QWidget* parent)
	: QWidget(parent), ui(new Ui::ExternalToolsPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
	ui->kcfg_JsonEditor->setClearButtonEnabled(true);
#endif

	ui->mceditLink->setOpenExternalLinks(true);
	ui->jvisualvmLink->setOpenExternalLinks(true);
	ui->jprofilerLink->setOpenExternalLinks(true);
}

ExternalToolsPage::~ExternalToolsPage()
{
	delete ui;
}

void ExternalToolsPage::applySettings()
{
	// The paths are bound to their settings by the settings dialog (the
	// widgets named kcfg_<ID>); what is left is to turn the name of the editor
	// into the path of its executable.
	QString jsonEditor = ui->kcfg_JsonEditor->text();
	if (!jsonEditor.isEmpty() && (!QFileInfo(jsonEditor).exists() ||
								  !QFileInfo(jsonEditor).isExecutable())) {
		QString found = QStandardPaths::findExecutable(jsonEditor);
		if (!found.isEmpty()) {
			jsonEditor = found;
		}
	}
	ui->kcfg_JsonEditor->setText(jsonEditor);
	APPLICATION->globalSettings()->setJsonEditor(jsonEditor);
	APPLICATION->globalSettings()->save();
}

void ExternalToolsPage::on_jprofilerPathBtn_clicked()
{
	QString raw_dir = ui->kcfg_JProfilerPath->text();
	QString error;
	do {
		raw_dir = QFileDialog::getExistingDirectory(
			this, i18n("JProfiler Folder"), raw_dir);
		if (raw_dir.isEmpty()) {
			break;
		}
		QString cooked_dir = FS::NormalizePath(raw_dir);
		if (!APPLICATION->profilers()["jprofiler"]->check(cooked_dir, &error)) {
			QMessageBox::critical(
				this, i18n("Error"),
				i18n("Error while checking JProfiler install:\n%1", error));
			continue;
		} else {
			ui->kcfg_JProfilerPath->setText(cooked_dir);
			break;
		}
	} while (1);
}
void ExternalToolsPage::on_jprofilerCheckBtn_clicked()
{
	QString error;
	if (!APPLICATION->profilers()["jprofiler"]->check(
			ui->kcfg_JProfilerPath->text(), &error)) {
		QMessageBox::critical(
			this, i18n("Error"),
			i18n("Error while checking JProfiler install:\n%1", error));
	} else {
		QMessageBox::information(this, i18n("OK"),
								 i18n("JProfiler setup seems to be OK"));
	}
}

void ExternalToolsPage::on_jvisualvmPathBtn_clicked()
{
	QString raw_dir = ui->kcfg_JVisualVMPath->text();
	QString error;
	do {
		raw_dir = QFileDialog::getOpenFileName(this, i18n("JVisualVM Executable"),
											   raw_dir);
		if (raw_dir.isEmpty()) {
			break;
		}
		QString cooked_dir = FS::NormalizePath(raw_dir);
		if (!APPLICATION->profilers()["jvisualvm"]->check(cooked_dir, &error)) {
			QMessageBox::critical(
				this, i18n("Error"),
				i18n("Error while checking JVisualVM install:\n%1", error));
			continue;
		} else {
			ui->kcfg_JVisualVMPath->setText(cooked_dir);
			break;
		}
	} while (1);
}
void ExternalToolsPage::on_jvisualvmCheckBtn_clicked()
{
	QString error;
	if (!APPLICATION->profilers()["jvisualvm"]->check(
			ui->kcfg_JVisualVMPath->text(), &error)) {
		QMessageBox::critical(
			this, i18n("Error"),
			i18n("Error while checking JVisualVM install:\n%1", error));
	} else {
		QMessageBox::information(this, i18n("OK"),
								 i18n("JVisualVM setup seems to be OK"));
	}
}

void ExternalToolsPage::on_mceditPathBtn_clicked()
{
	QString raw_dir = ui->kcfg_MCEditPath->text();
	QString error;
	do {
#ifdef Q_OS_MACOS
		raw_dir = QFileDialog::getOpenFileName(this, i18n("MCEdit Application"),
											   raw_dir);
#else
		raw_dir = QFileDialog::getExistingDirectory(this, i18n("MCEdit Folder"),
													raw_dir);
#endif
		if (raw_dir.isEmpty()) {
			break;
		}
		QString cooked_dir = FS::NormalizePath(raw_dir);
		if (!APPLICATION->mcedit()->check(cooked_dir, error)) {
			QMessageBox::critical(
				this, i18n("Error"),
				i18n("Error while checking MCEdit install:\n%1", error));
			continue;
		} else {
			ui->kcfg_MCEditPath->setText(cooked_dir);
			break;
		}
	} while (1);
}
void ExternalToolsPage::on_mceditCheckBtn_clicked()
{
	QString error;
	if (!APPLICATION->mcedit()->check(ui->kcfg_MCEditPath->text(), error)) {
		QMessageBox::critical(
			this, i18n("Error"),
			i18n("Error while checking MCEdit install:\n%1", error));
	} else {
		QMessageBox::information(this, i18n("OK"),
								 i18n("MCEdit setup seems to be OK"));
	}
}

void ExternalToolsPage::on_jsonEditorBrowseBtn_clicked()
{
	QString raw_file =
		QFileDialog::getOpenFileName(this, i18n("JSON Editor"),
									 ui->kcfg_JsonEditor->text().isEmpty()
#if defined(Q_OS_LINUX)
										 ? QString("/usr/bin")
#else
										 ? QStandardPaths::standardLocations(
											   QStandardPaths::
												   ApplicationsLocation)
											   .first()
#endif
										 : ui->kcfg_JsonEditor->text());

	if (raw_file.isEmpty()) {
		return;
	}
	QString cooked_file = FS::NormalizePath(raw_file);

	// it has to exist and be an executable
	if (QFileInfo(cooked_file).exists() &&
		QFileInfo(cooked_file).isExecutable()) {
		ui->kcfg_JsonEditor->setText(cooked_file);
	} else {
		QMessageBox::warning(
			this, i18n("Invalid"),
			i18n("The file chosen does not seem to be an executable"));
	}
}

bool ExternalToolsPage::apply()
{
	applySettings();
	return true;
}
