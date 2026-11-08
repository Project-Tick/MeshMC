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

#include "MeshMCPage.h"
#include "ui_MeshMCPage.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QTextCharFormat>

#include "updater/ExternalUpdater.h"

#include "settings/GlobalSettings.h"
#include "settings/SettingsObject.h"
#include "ui/widgets/ChoiceBinding.h"
#include <FileSystem.h>
#include "InstanceList.h"
#include "DesktopServices.h"
#include "Application.h"
#include "BuildConfig.h"

#include <QApplication>
#include <QProcess>
#include <KLocalizedString>

MeshMCPage::MeshMCPage(QWidget* parent)
	: QWidget(parent), ui(new Ui::MeshMCPage)
{
	ui->setupUi(this);
	auto origForeground =
		ui->fontPreview->palette().color(ui->fontPreview->foregroundRole());
	auto origBackground =
		ui->fontPreview->palette().color(ui->fontPreview->backgroundRole());
	m_colors.reset(new LogColorCache(origForeground, origBackground));

	// The sorting is a choice of radio buttons, a text in the settings. The
	// settings dialog binds it, like the widgets named kcfg_<ID>.
	auto* sortMode = new ChoiceBinding("InstSortMode", this);
	sortMode->addChoice(ui->sortByNameBtn, "Name");
	sortMode->addChoice(ui->sortLastLaunchedBtn, "LastLaunch");

	defaultFormat = new QTextCharFormat(ui->fontPreview->currentCharFormat());

	loadSettings();

	ui->updateSettingsBox->setHidden(!APPLICATION->updater());
	connect(ui->kcfg_ConsoleFontSize, qOverload<int>(&QSpinBox::valueChanged), this,
			&MeshMCPage::refreshFontPreview);
	connect(ui->kcfg_ConsoleFont, &QFontComboBox::currentFontChanged, this,
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

bool MeshMCPage::confirmInstanceDirPath(const QString& rawDir,
									   const QString& cookedDir)
{
	if (FS::checkProblemticPathJava(QDir(cookedDir))) {
		QMessageBox warning;
		warning.setText(
			i18n("You're trying to specify an instance folder which\'s path "
			   "contains at least one \'!\'. "
			   "Java is known to cause problems if that is the case, your "
			   "instances (probably) won't start!"));
		warning.setInformativeText(
			i18n("Do you really want to use this path? "
			   "Selecting \"No\" will close this and not alter your "
			   "instance path."));
		warning.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		return warning.exec() == QMessageBox::Yes;
	}

	/* A folder handed to us through a Flatpak portal is borrowed, not
	 * granted.
	 *
	 * The sandbox exposes a one-off pick under /run/user for the lifetime
	 * of the session. Store it as an instance folder and it works
	 * perfectly until the next restart, at which point the launcher can no
	 * longer see it - and the instances in it read as deleted. The raw path
	 * is what carries this evidence, which is why normalising happens
	 * separately: the cooked path no longer says where it came from.
	 */
	if (DesktopServices::isFlatpak() && rawDir.startsWith("/run/user")) {
		QMessageBox warning;
		warning.setText(
			i18n("You're trying to specify an instance folder "
			   "which was granted temporarily via Flatpak.\n"
			   "This is known to cause problems. "
			   "After a restart the launcher might break, "
			   "because it will no longer have access to that directory.\n\n"
			   "Granting MeshMC access to it via Flatseal is recommended."));
		warning.setInformativeText(i18n("Do you want to proceed anyway?"));
		warning.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		return warning.exec() == QMessageBox::Yes;
	}

	return true;
}

void MeshMCPage::on_instDirBrowseBtn_clicked()
{
	QString rawDir = QFileDialog::getExistingDirectory(
		this, i18n("Instance Folder"), ui->kcfg_InstanceDir->text());

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!rawDir.isEmpty() && QDir(rawDir).exists()) {
		QString cookedDir = FS::NormalizePath(rawDir);
		if (confirmInstanceDirPath(rawDir, cookedDir)) {
			ui->kcfg_InstanceDir->setText(cookedDir);
		}
	}
}

QStringList MeshMCPage::additionalInstanceDirs() const
{
	QStringList dirs;
	for (int i = 0; i < ui->additionalInstDirsList->count(); i++) {
		dirs << ui->additionalInstDirsList->item(i)->text();
	}
	return dirs;
}

void MeshMCPage::on_addInstDirBtn_clicked()
{
	QString rawDir = QFileDialog::getExistingDirectory(
		this, i18n("Additional Instance Folder"));
	if (rawDir.isEmpty() || !QDir(rawDir).exists()) {
		return;
	}

	QString cookedDir = FS::NormalizePath(rawDir);

	/* Refuse a folder that is already in play, primary or additional, and
	 * do it before asking anything else.
	 *
	 * Two entries for one folder is not merely redundant: discovery walks
	 * each configured root and keeps the first claim on an instance id, so
	 * a second pass over the same folder reports every instance in it as a
	 * duplicate and skips it. The list would look richer while instances
	 * went missing.
	 *
	 * Checked ahead of confirmInstanceDirPath() so that a folder being
	 * already known is the whole answer - there is no point warning about
	 * a '!' in a path we are about to refuse for a different reason.
	 *
	 * Matched with MatchFixedString, which ignores case: on a
	 * case-insensitive filesystem two spellings are one folder, and
	 * InstanceList would collapse them anyway - better to say so here,
	 * where there is somebody to tell.
	 */
	if (cookedDir == FS::NormalizePath(ui->kcfg_InstanceDir->text())) {
		QMessageBox::warning(
			this, i18n("Duplicate directory"),
			i18n("This is already your primary instance directory."));
		return;
	}
	if (!ui->additionalInstDirsList
			 ->findItems(cookedDir, Qt::MatchFixedString)
			 .isEmpty()) {
		QMessageBox::warning(this, i18n("Duplicate directory"),
							 i18n("This directory has already been added."));
		return;
	}

	if (!confirmInstanceDirPath(rawDir, cookedDir)) {
		return;
	}

	ui->additionalInstDirsList->addItem(cookedDir);
}

void MeshMCPage::on_removeInstDirBtn_clicked()
{
	/* Deletes the selection rather than one row by index. The list is
	 * single-selection today, so this removes exactly one item; written
	 * this way it keeps working if the list is ever allowed to take more,
	 * and it does nothing when nothing is selected. */
	qDeleteAll(ui->additionalInstDirsList->selectedItems());
}

void MeshMCPage::on_iconsDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(
		this, i18n("Icons Folder"), ui->kcfg_IconsDir->text());

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!raw_dir.isEmpty() && QDir(raw_dir).exists()) {
		QString cooked_dir = FS::NormalizePath(raw_dir);
		ui->kcfg_IconsDir->setText(cooked_dir);
	}
}
void MeshMCPage::on_skinsDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(
		this, i18n("Skins Folder"), ui->kcfg_SkinsDir->text());

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!raw_dir.isEmpty() && QDir(raw_dir).exists()) {
		QString cooked_dir = FS::NormalizePath(raw_dir);
		ui->kcfg_SkinsDir->setText(cooked_dir);
	}
}
void MeshMCPage::on_javaDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(
		this, i18n("Java Folder"), ui->kcfg_JavaDir->text());

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!raw_dir.isEmpty() && QDir(raw_dir).exists()) {
		QString cooked_dir = FS::NormalizePath(raw_dir);
		ui->kcfg_JavaDir->setText(cooked_dir);
	}
}
void MeshMCPage::on_modsDirBrowseBtn_clicked()
{
	QString raw_dir = QFileDialog::getExistingDirectory(
		this, i18n("Mods Folder"), ui->kcfg_CentralModsDir->text());

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (!raw_dir.isEmpty() && QDir(raw_dir).exists()) {
		QString cooked_dir = FS::NormalizePath(raw_dir);
		ui->kcfg_CentralModsDir->setText(cooked_dir);
	}
}
void MeshMCPage::on_migrateDataFolderMacBtn_clicked()
{
	QMessageBox::information(
		this, i18n("Automatic macOS Migration"),
		i18n("MeshMC now stores macOS data under your Library/Application Support "
		   "folder automatically."));
}

void MeshMCPage::refreshUpdateChannelList()
{
	// No-op: the new updater does not use named channels.
}

void MeshMCPage::openedImpl()
{
	// The widgets are bound to the settings by the dialog after the page has
	// been built, so the preview is drawn from what they show now.
	refreshFontPreview();
}

// Everything on this page that is a plain value in a widget named
// kcfg_<Setting> (the console, the folders, the backup and the game files) is
// bound to its setting by the settings dialog. What is left cannot be: the
// values that are not a setting of their own, or that are stored differently
// from how they are shown.
void MeshMCPage::applySettings()
{
	auto* s = APPLICATION->globalSettings();

	if (ui->resetNotificationsBtn->isChecked()) {
		s->setShownNotifications(QString());
	}

	if (auto* updater = APPLICATION->updater()) {
		updater->setAutomaticallyChecksForUpdates(
			ui->autoUpdateCheckBox->isChecked());
		updater->setUpdateCheckInterval(ui->updateIntervalSpinBox->value() *
										3600);
	}

	/* The modpack prompt is stored inverted - the setting says what to
	 * *skip* - so that a launcher nobody has configured asks, which is the
	 * useful default and the behaviour that existed before it was optional. */
	s->setSkipModpackUpdatePrompt(!ui->modpackUpdatePromptCheck->isChecked());

	/* Encoded, not handed over as a list: every value is stored as a string,
	 * and a multi-element QStringList stringifies to nothing at all.
	 * InstanceList owns that detail. */
	s->setAdditionalInstanceDirs(
		InstanceList::encodeInstanceDirList(additionalInstanceDirs())
			.toString());

	s->save();
}

void MeshMCPage::loadSettings()
{
	auto* s = APPLICATION->globalSettings();
	// Updates. They do not come from the launcher's settings: the updater
	// keeps its own.
	if (auto* updater = APPLICATION->updater()) {
		ui->autoUpdateCheckBox->setChecked(
			updater->getAutomaticallyChecksForUpdates());
		ui->updateIntervalSpinBox->setValue(
			static_cast<int>(updater->getUpdateCheckInterval() / 3600));
	}

	ui->modpackUpdatePromptCheck->setChecked(
		!s->skipModpackUpdatePrompt());

	ui->additionalInstDirsList->clear();
	ui->additionalInstDirsList->addItems(InstanceList::decodeInstanceDirList(
		s->additionalInstanceDirs()));
}

void MeshMCPage::refreshFontPreview()
{
	int fontSize = ui->kcfg_ConsoleFontSize->value();
	QString fontFamily = ui->kcfg_ConsoleFont->currentFont().family();
	ui->fontPreview->clear();
	defaultFormat->setFont(QFont(fontFamily, fontSize));
	{
		QTextCharFormat format(*defaultFormat);
		format.setForeground(m_colors->getFront(MessageLevel::Error));
		// append a paragraph/line
		auto workCursor = ui->fontPreview->textCursor();
		workCursor.movePosition(QTextCursor::End);
		workCursor.insertText(i18n("[Something/ERROR] A spooky error!"), format);
		workCursor.insertBlock();
	}
	{
		QTextCharFormat format(*defaultFormat);
		format.setForeground(m_colors->getFront(MessageLevel::Message));
		// append a paragraph/line
		auto workCursor = ui->fontPreview->textCursor();
		workCursor.movePosition(QTextCursor::End);
		workCursor.insertText(i18n("[Test/INFO] A harmless message..."), format);
		workCursor.insertBlock();
	}
	{
		QTextCharFormat format(*defaultFormat);
		format.setForeground(m_colors->getFront(MessageLevel::Warning));
		// append a paragraph/line
		auto workCursor = ui->fontPreview->textCursor();
		workCursor.movePosition(QTextCursor::End);
		workCursor.insertText(i18n("[Something/WARN] A not so spooky warning."),
							  format);
		workCursor.insertBlock();
	}
}
