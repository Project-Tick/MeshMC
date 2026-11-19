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

#include "ImportPage.h"
#include "ui_ImportPage.h"

#include <QFileDialog>
#include <QValidator>

#include "ui/dialogs/NewInstanceDialog.h"

#include "InstanceImportTask.h"

class UrlValidator : public QValidator
{
  public:
	using QValidator::QValidator;

	State validate(QString& in, int& pos) const
	{
		const QUrl url(in);
		if (url.isValid() && !url.isRelative() && !url.isEmpty()) {
			return Acceptable;
		} else if (QFile::exists(in)) {
			return Acceptable;
		} else {
			return Intermediate;
		}
	}
};

ImportPage::ImportPage(NewInstanceDialog* dialog, QWidget* parent)
	: QWidget(parent), ui(new Ui::ImportPage), dialog(dialog)
{
	ui->setupUi(this);
	ui->modpackEdit->setValidator(new UrlValidator(ui->modpackEdit));
	connect(ui->modpackEdit, &QLineEdit::textChanged, this,
			&ImportPage::updateState);
}

ImportPage::~ImportPage()
{
	delete ui;
}

bool ImportPage::shouldDisplay() const
{
	return true;
}

void ImportPage::openedImpl()
{
	updateState();
}

void ImportPage::updateState()
{
	if (!isOpened) {
		return;
	}
	if (ui->modpackEdit->hasAcceptableInput()) {
		QString input = ui->modpackEdit->text();
		auto url = QUrl::fromUserInput(input);
		if (url.isLocalFile()) {
			// Accept every common modpack-archive extension. The real
			// format sniff (mrpack vs CurseForge zip vs MultiMC zip vs
			// Technic) happens later inside InstanceImportTask, which
			// scans the archive's entry list — extension is only a hint.
			QFileInfo fi(input);
			const QString suffix = fi.suffix().toLower();
			const bool looksLikeArchive = suffix == QStringLiteral("zip") ||
										  suffix == QStringLiteral("mrpack") ||
										  suffix == QStringLiteral("jar");
			if (fi.exists() && looksLikeArchive) {
				QFileInfo nameFi(url.fileName());
				dialog->setSuggestedPack(nameFi.completeBaseName(),
										 new InstanceImportTask(url));
				dialog->setSuggestedIcon("default");
			}
		} else {
			if (input.endsWith("?client=y")) {
				input.chop(9);
				input.append("/file");
				url = QUrl::fromUserInput(input);
			}
			// hook, line and sinker.
			QFileInfo fi(url.fileName());
			dialog->setSuggestedPack(fi.completeBaseName(),
									 new InstanceImportTask(url));
			dialog->setSuggestedIcon("default");
		}
	} else {
		dialog->setSuggestedPack();
	}
}

void ImportPage::setUrl(const QString& url)
{
	ui->modpackEdit->setText(url);
	updateState();
}

void ImportPage::on_modpackBtn_clicked()
{
	// The launcher's InstanceImportTask understands four archive
	// dialects (CurseForge zip, Modrinth .mrpack, MultiMC zip,
	// Technic .zip). Expose all of them in the file dialog so the
	// user can pick a .mrpack without renaming it first.
	const QString filter = tr("Modpack archives (*.zip *.mrpack);;"
							  "Modrinth packs (*.mrpack);;"
							  "Zip archives (*.zip);;"
							  "All files (*)");
	const QUrl url = QFileDialog::getOpenFileUrl(this, tr("Choose modpack"),
												 modpackUrl(), filter);
	if (url.isValid()) {
		if (url.isLocalFile()) {
			ui->modpackEdit->setText(url.toLocalFile());
		} else {
			ui->modpackEdit->setText(url.toString());
		}
	}
}

QUrl ImportPage::modpackUrl() const
{
	const QUrl url(ui->modpackEdit->text());
	if (url.isValid() && !url.isRelative() && !url.host().isEmpty()) {
		return url;
	} else {
		return QUrl::fromLocalFile(ui->modpackEdit->text());
	}
}
