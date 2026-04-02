/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QFileInfo>
#include <QFileDialog>
#include <QPainter>
#include <QRegularExpression>

#include <FileSystem.h>

#include <minecraft/services/SkinUpload.h>
#include <minecraft/services/CapeChange.h>
#include <tasks/SequentialTask.h>

#include "SkinUploadDialog.h"
#include "ui_SkinUploadDialog.h"
#include "ProgressDialog.h"
#include "CustomMessageBox.h"

void SkinUploadDialog::on_buttonBox_rejected()
{
	close();
}

void SkinUploadDialog::on_buttonBox_accepted()
{
	QString fileName;
	QString input = ui->skinPathTextBox->text();
	QRegularExpression urlPrefixMatcher("^([a-z]+)://.+$");
	bool isLocalFile = false;
	// it has an URL prefix -> it is an URL
	if (urlPrefixMatcher.match(input).hasMatch()) {
		QUrl fileURL = input;
		if (fileURL.isValid()) {
			// local?
			if (fileURL.isLocalFile()) {
				isLocalFile = true;
				fileName = fileURL.toLocalFile();
			} else {
				CustomMessageBox::selectable(
					this, tr("Skin Upload"),
					tr("Using remote URLs for setting skins is not implemented "
					   "yet."),
					QMessageBox::Warning)
					->exec();
				close();
				return;
			}
		} else {
			CustomMessageBox::selectable(
				this, tr("Skin Upload"),
				tr("You cannot use an invalid URL for uploading skins."),
				QMessageBox::Warning)
				->exec();
			close();
			return;
		}
	} else {
		// just assume it's a path then
		isLocalFile = true;
		fileName = ui->skinPathTextBox->text();
	}
	if (isLocalFile && !QFile::exists(fileName)) {
		CustomMessageBox::selectable(this, tr("Skin Upload"),
									 tr("Skin file does not exist!"),
									 QMessageBox::Warning)
			->exec();
		close();
		return;
	}
	SkinUpload::Model model = SkinUpload::STEVE;
	if (ui->steveBtn->isChecked()) {
		model = SkinUpload::STEVE;
	} else if (ui->alexBtn->isChecked()) {
		model = SkinUpload::ALEX;
	}
	ProgressDialog prog(this);
	SequentialTask skinUpload;
	skinUpload.addTask(shared_qobject_ptr<SkinUpload>(new SkinUpload(
		this, m_acct->accessToken(), FS::read(fileName), model)));
	auto selectedCape = ui->capeCombo->currentData().toString();
	if (selectedCape != m_acct->accountData()->minecraftProfile.currentCape) {
		skinUpload.addTask(shared_qobject_ptr<CapeChange>(
			new CapeChange(this, m_acct->accessToken(), selectedCape)));
	}
	if (prog.execWithTask(&skinUpload) != QDialog::Accepted) {
		CustomMessageBox::selectable(this, tr("Skin Upload"),
									 tr("Failed to upload skin!"),
									 QMessageBox::Warning)
			->exec();
		close();
		return;
	}
	CustomMessageBox::selectable(this, tr("Skin Upload"), tr("Success"),
								 QMessageBox::Information)
		->exec();
	close();
}

void SkinUploadDialog::on_skinBrowseBtn_clicked()
{
	QString raw_path = QFileDialog::getOpenFileName(
		this, tr("Select Skin Texture"), QString(), "*.png");
	if (raw_path.isEmpty() || !QFileInfo::exists(raw_path)) {
		return;
	}
	QString cooked_path = FS::NormalizePath(raw_path);
	ui->skinPathTextBox->setText(cooked_path);
}

SkinUploadDialog::SkinUploadDialog(MinecraftAccountPtr acct, QWidget* parent)
	: QDialog(parent), m_acct(acct), ui(new Ui::SkinUploadDialog)
{
	ui->setupUi(this);

	// FIXME: add a model for this, download/refresh the capes on demand
	auto& data = *acct->accountData();
	int index = 0;
	ui->capeCombo->addItem(tr("No Cape"), QVariant());
	auto currentCape = data.minecraftProfile.currentCape;
	if (currentCape.isEmpty()) {
		ui->capeCombo->setCurrentIndex(index);
	}

	for (auto& cape : data.minecraftProfile.capes) {
		index++;
		if (cape.data.size()) {
			QPixmap capeImage;
			if (capeImage.loadFromData(cape.data, "PNG")) {
				QPixmap preview = QPixmap(10, 16);
				QPainter painter(&preview);
				painter.drawPixmap(0, 0, capeImage.copy(1, 1, 10, 16));
				ui->capeCombo->addItem(capeImage, cape.alias, cape.id);
				if (currentCape == cape.id) {
					ui->capeCombo->setCurrentIndex(index);
				}
				continue;
			}
		}
		ui->capeCombo->addItem(cape.alias, cape.id);
		if (currentCape == cape.id) {
			ui->capeCombo->setCurrentIndex(index);
		}
	}
}
