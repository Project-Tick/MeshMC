/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
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
			// FIXME: actually do some validation of what's inside here... this
			// is fake AF
			QFileInfo fi(input);
			if (fi.exists() && fi.suffix() == "zip") {
				QFileInfo fi(url.fileName());
				dialog->setSuggestedPack(fi.completeBaseName(),
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
	const QUrl url = QFileDialog::getOpenFileUrl(
		this, tr("Choose modpack"), modpackUrl(), tr("Zip (*.zip)"));
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
