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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include "MeshMCPage.h"
#include "ui_MeshMCPage.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QTextCharFormat>

#include "updater/UpdateChecker.h"

#include "settings/SettingsObject.h"
#include <FileSystem.h>
#include "Application.h"
#include "BuildConfig.h"

#include <QApplication>
#include <QProcess>

// FIXME: possibly move elsewhere
enum InstSortMode {
	// Sort alphabetically by name.
	Sort_Name,
	// Sort by which instance was launched most recently.
	Sort_LastLaunch
};

MeshMCPage::MeshMCPage(QWidget* parent)
	: QWidget(parent), ui(new Ui::MeshMCPage)
{
	ui->setupUi(this);
	auto origForeground =
		ui->fontPreview->palette().color(ui->fontPreview->foregroundRole());
	auto origBackground =
		ui->fontPreview->palette().color(ui->fontPreview->backgroundRole());
	m_colors.reset(new LogColorCache(origForeground, origBackground));

	ui->sortingModeGroup->setId(ui->sortByNameBtn, Sort_Name);
	ui->sortingModeGroup->setId(ui->sortLastLaunchedBtn, Sort_LastLaunch);

	defaultFormat = new QTextCharFormat(ui->fontPreview->currentCharFormat());

	m_languageModel = APPLICATION->translations();
	loadSettings();

	if (BuildConfig.UPDATER_ENABLED && UpdateChecker::isUpdaterSupported()) {
		// New updater: hide the legacy channel selector (no channel selection
		// in the new system).
		ui->updateChannelComboBox->setVisible(false);
		ui->updateChannelLabel->setVisible(false);
		ui->updateChannelDescLabel->setVisible(false);
	} else {
		ui->updateSettingsBox->setHidden(true);
	}
	// Analytics
	if (BuildConfig.ANALYTICS_ID.isEmpty()) {
		ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->analyticsTab));
	}
	connect(ui->fontSizeBox, &QSpinBox::valueChanged, this,
			&MeshMCPage::refreshFontPreview);
	connect(ui->consoleFont, &QFontComboBox::currentFontChanged, this,
			&MeshMCPage::refreshFontPreview);

	ui->migrateDataFolderMacBtn->setVisible(false);
}

MeshMCPage::~MeshMCPage()
{
	delete ui;
	delete defaultFormat;
}

bool MeshMCPage::apply()
{
	applySettings();
	return true;
}

void MeshMCPage::on_instDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(
		this, tr("Instance Folder"), ui->instDirTextBox->text());

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!raw_dir.isEmpty() && QDir(raw_dir).exists()) {
		QString cooked_dir = FS::NormalizePath(raw_dir);
		if (FS::checkProblemticPathJava(QDir(cooked_dir))) {
			QMessageBox warning;
			warning.setText(
				tr("You're trying to specify an instance folder which\'s path "
				   "contains at least one \'!\'. "
				   "Java is known to cause problems if that is the case, your "
				   "instances (probably) won't start!"));
			warning.setInformativeText(
				tr("Do you really want to use this path? "
				   "Selecting \"No\" will close this and not alter your "
				   "instance path."));
			warning.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			int result = warning.exec();
			if (result == QMessageBox::Yes) {
				ui->instDirTextBox->setText(cooked_dir);
			}
		} else {
			ui->instDirTextBox->setText(cooked_dir);
		}
	}
}

void MeshMCPage::on_iconsDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(
		this, tr("Icons Folder"), ui->iconsDirTextBox->text());

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!raw_dir.isEmpty() && QDir(raw_dir).exists()) {
		QString cooked_dir = FS::NormalizePath(raw_dir);
		ui->iconsDirTextBox->setText(cooked_dir);
	}
}
void MeshMCPage::on_modsDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(
		this, tr("Mods Folder"), ui->modsDirTextBox->text());

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!raw_dir.isEmpty() && QDir(raw_dir).exists()) {
		QString cooked_dir = FS::NormalizePath(raw_dir);
		ui->modsDirTextBox->setText(cooked_dir);
	}
}
void MeshMCPage::on_migrateDataFolderMacBtn_clicked()
{
	QMessageBox::information(
		this, tr("Automatic macOS Migration"),
		tr("%1 now stores macOS data under your Library/Application Support "
		   "folder automatically.")
			.arg(BuildConfig.MESHMC_DISPLAYNAME));
}

void MeshMCPage::refreshUpdateChannelList()
{
	// No-op: the new updater does not use named channels.
}

void MeshMCPage::updateChannelSelectionChanged(int)
{
	// No-op.
}

void MeshMCPage::refreshUpdateChannelDesc()
{
	// No-op.
}

void MeshMCPage::applySettings()
{
	auto s = APPLICATION->settings();

	if (ui->resetNotificationsBtn->isChecked()) {
		s->set("ShownNotifications", QString());
	}

	// Updates
	s->set("AutoUpdate", ui->autoUpdateCheckBox->isChecked());
	// (UpdateChannel setting removed - the new updater always checks the stable
	// feed)

	// Console settings
	s->set("ShowConsole", ui->showConsoleCheck->isChecked());
	s->set("AutoCloseConsole", ui->autoCloseConsoleCheck->isChecked());
	s->set("ShowConsoleOnError", ui->showConsoleErrorCheck->isChecked());
	QString consoleFontFamily = ui->consoleFont->currentFont().family();
	s->set("ConsoleFont", consoleFontFamily);
	s->set("ConsoleFontSize", ui->fontSizeBox->value());
	s->set("ConsoleMaxLines", ui->lineLimitSpinBox->value());
	s->set("ConsoleOverflowStop",
		   ui->checkStopLogging->checkState() != Qt::Unchecked);

	// Folders
	// TODO: Offer to move instances to new instance folder.
	s->set("InstanceDir", ui->instDirTextBox->text());
	s->set("CentralModsDir", ui->modsDirTextBox->text());
	s->set("IconsDir", ui->iconsDirTextBox->text());

	auto sortMode = (InstSortMode)ui->sortingModeGroup->checkedId();
	switch (sortMode) {
		case Sort_LastLaunch:
			s->set("InstSortMode", "LastLaunch");
			break;
		case Sort_Name:
		default:
			s->set("InstSortMode", "Name");
			break;
	}

	// Analytics
	if (!BuildConfig.ANALYTICS_ID.isEmpty()) {
		s->set("Analytics", ui->analyticsCheck->isChecked());
	}
}
void MeshMCPage::loadSettings()
{
	auto s = APPLICATION->settings();
	// Updates
	ui->autoUpdateCheckBox->setChecked(s->get("AutoUpdate").toBool());
	// (no channel to read in the new updater system)

	// Console settings
	ui->showConsoleCheck->setChecked(s->get("ShowConsole").toBool());
	ui->autoCloseConsoleCheck->setChecked(s->get("AutoCloseConsole").toBool());
	ui->showConsoleErrorCheck->setChecked(
		s->get("ShowConsoleOnError").toBool());
	QString fontFamily = APPLICATION->settings()->get("ConsoleFont").toString();
	QFont consoleFont(fontFamily);
	ui->consoleFont->setCurrentFont(consoleFont);

	bool conversionOk = true;
	int fontSize =
		APPLICATION->settings()->get("ConsoleFontSize").toInt(&conversionOk);
	if (!conversionOk) {
		fontSize = 11;
	}
	ui->fontSizeBox->setValue(fontSize);
	refreshFontPreview();
	ui->lineLimitSpinBox->setValue(s->get("ConsoleMaxLines").toInt());
	ui->checkStopLogging->setChecked(s->get("ConsoleOverflowStop").toBool());

	// Folders
	ui->instDirTextBox->setText(s->get("InstanceDir").toString());
	ui->modsDirTextBox->setText(s->get("CentralModsDir").toString());
	ui->iconsDirTextBox->setText(s->get("IconsDir").toString());

	QString sortMode = s->get("InstSortMode").toString();

	if (sortMode == "LastLaunch") {
		ui->sortLastLaunchedBtn->setChecked(true);
	} else {
		ui->sortByNameBtn->setChecked(true);
	}

	// Analytics
	if (!BuildConfig.ANALYTICS_ID.isEmpty()) {
		ui->analyticsCheck->setChecked(s->get("Analytics").toBool());
	}
}

void MeshMCPage::refreshFontPreview()
{
	int fontSize = ui->fontSizeBox->value();
	QString fontFamily = ui->consoleFont->currentFont().family();
	ui->fontPreview->clear();
	defaultFormat->setFont(QFont(fontFamily, fontSize));
	{
		QTextCharFormat format(*defaultFormat);
		format.setForeground(m_colors->getFront(MessageLevel::Error));
		// append a paragraph/line
		auto workCursor = ui->fontPreview->textCursor();
		workCursor.movePosition(QTextCursor::End);
		workCursor.insertText(tr("[Something/ERROR] A spooky error!"), format);
		workCursor.insertBlock();
	}
	{
		QTextCharFormat format(*defaultFormat);
		format.setForeground(m_colors->getFront(MessageLevel::Message));
		// append a paragraph/line
		auto workCursor = ui->fontPreview->textCursor();
		workCursor.movePosition(QTextCursor::End);
		workCursor.insertText(tr("[Test/INFO] A harmless message..."), format);
		workCursor.insertBlock();
	}
	{
		QTextCharFormat format(*defaultFormat);
		format.setForeground(m_colors->getFront(MessageLevel::Warning));
		// append a paragraph/line
		auto workCursor = ui->fontPreview->textCursor();
		workCursor.movePosition(QTextCursor::End);
		workCursor.insertText(tr("[Something/WARN] A not so spooky warning."),
							  format);
		workCursor.insertBlock();
	}
}
