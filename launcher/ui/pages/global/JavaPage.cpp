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

#include "JavaPage.h"
#include "JavaCommon.h"
#include "ui_JavaPage.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QTabBar>
#include <QTreeWidgetItem>
#include <QDirIterator>

#include "ui/dialogs/VersionSelectDialog.h"
#ifndef MeshMC_DISABLE_JAVA_DOWNLOADER
#include "ui/dialogs/JavaDownloadDialog.h"
#endif

#include "java/JavaUtils.h"
#include "java/JavaInstallList.h"
#include "java/download/JavaRuntime.h"

#include "settings/SettingsObject.h"
#include <FileSystem.h>
#include "Application.h"
#include "settings/GlobalSettings.h"
#include "ui/widgets/ChoiceBinding.h"
#include "Sys.h"
#include <KLocalizedString>

JavaPage::JavaPage(QWidget* parent) : QWidget(parent), ui(new Ui::JavaPage)
{
	ui->setupUi(this);

	auto sysMiB = Sys::getSystemRam() / Sys::mebibyte;
	ui->kcfg_MaxMemAlloc->setMaximum(sysMiB);

	// Populate vendor combobox
	auto providers = JavaDownload::JavaProviderInfo::availableProviders();
	for (const auto& provider : providers) {
		ui->javaAutoDownloadVendorBox->addItem(provider.name, provider.uid);
	}

	// The vendor is the ID of a provider, not the name that is shown, which
	// the settings dialog cannot bind by itself.
	auto* vendor = new ChoiceBinding("JavaAutoDownloadVendor", this);
	vendor->setComboBox(ui->javaAutoDownloadVendorBox);

#ifdef MeshMC_DISABLE_JAVA_DOWNLOADER
	// Hide the entire Installations tab when Java downloader is disabled
	int idx = ui->tabWidget->indexOf(ui->tabInstallations);
	if (idx != -1)
		ui->tabWidget->removeTab(idx);
	// Hide vendor selector and auto-download toggle when downloader is disabled
	ui->kcfg_JavaAutoDownload->setVisible(false);
	ui->labelAutoDownloadVendor->setVisible(false);
	ui->javaAutoDownloadVendorBox->setVisible(false);
#else
	refreshInstalledJavas();
#endif
}

JavaPage::~JavaPage()
{
	delete ui;
}

bool JavaPage::apply()
{
	applySettings();
	return true;
}

void JavaPage::applySettings()
{
	auto* s = APPLICATION->globalSettings();

	// The memory, the paths, the vendor and the arguments are bound to their settings by
	// the settings dialog (the widgets named kcfg_<ID>), which has saved them
	// when this is called. What is left: the smaller of the two memory
	// values is the minimum,
	int min = ui->kcfg_MinMemAlloc->value();
	int max = ui->kcfg_MaxMemAlloc->value();
	if (min > max) {
		ui->kcfg_MinMemAlloc->setValue(max);
		ui->kcfg_MaxMemAlloc->setValue(min);
		s->setMinMemAlloc(max);
		s->setMaxMemAlloc(min);
		s->save();
	}

	// and the arguments are checked.
	JavaCommon::checkJVMArgs(ui->kcfg_JvmArgs->text(), this->parentWidget());
}

void JavaPage::on_javaDetectBtn_clicked()
{
	JavaInstallPtr java;

	VersionSelectDialog vselect(APPLICATION->javalist().get(),
								i18n("Select a Java version"), this, true);
	vselect.setResizeOn(2);
	vselect.exec();

	if (vselect.result() == QDialog::Accepted && vselect.selectedVersion()) {
		java =
			std::dynamic_pointer_cast<JavaInstall>(vselect.selectedVersion());
		ui->kcfg_JavaPath->setText(java->path);
	}
}

void JavaPage::on_javaBrowseBtn_clicked()
{
	QString raw_path =
		QFileDialog::getOpenFileName(this, i18n("Find Java executable"));

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (raw_path.isEmpty()) {
		return;
	}

	QString cooked_path = FS::NormalizePath(raw_path);
	QFileInfo javaInfo(cooked_path);
	;
	if (!javaInfo.exists() || !javaInfo.isExecutable()) {
		return;
	}
	ui->kcfg_JavaPath->setText(cooked_path);
}

void JavaPage::on_javaTestBtn_clicked()
{
	if (checker) {
		return;
	}
	checker.reset(new JavaCommon::TestCheck(
		this, ui->kcfg_JavaPath->text(), ui->kcfg_JvmArgs->text(),
		ui->kcfg_MinMemAlloc->value(), ui->kcfg_MaxMemAlloc->value(),
		ui->kcfg_PermGen->value()));
	connect(checker.get(), &JavaCommon::TestCheck::finished, this,
			&JavaPage::checkerFinished);
	checker->run();
}

void JavaPage::checkerFinished()
{
	checker.reset();
}

void JavaPage::on_javaDownloadBtn_clicked()
{
#ifndef MeshMC_DISABLE_JAVA_DOWNLOADER
	JavaDownloadDialog dlg(this);
	if (dlg.exec() == QDialog::Accepted) {
		refreshInstalledJavas();
	}
#endif
}

void JavaPage::on_javaRefreshBtn_clicked()
{
	refreshInstalledJavas();
}

void JavaPage::on_javaRemoveBtn_clicked()
{
	auto* item = ui->installedJavaTree->currentItem();
	if (!item)
		return;

	QString path = item->text(2);
	if (path.isEmpty())
		return;

	// Find the java installation root directory (parent of bin/)
	QFileInfo fi(path);
	QDir javaDir = fi.dir(); // bin/
	javaDir.cdUp();			 // java root

	auto result = QMessageBox::question(
		this, i18n("Remove Java Installation"),
		i18n("Are you sure you want to remove this Java installation?\n\n%1", javaDir.absolutePath()),
		QMessageBox::Yes | QMessageBox::No);

	if (result != QMessageBox::Yes)
		return;

	javaDir.removeRecursively();
	refreshInstalledJavas();
}

void JavaPage::on_javaUseBtn_clicked()
{
	auto* item = ui->installedJavaTree->currentItem();
	if (!item)
		return;

	QString path = item->text(2);
	if (!path.isEmpty()) {
		ui->kcfg_JavaPath->setText(path);
		ui->tabWidget->setCurrentIndex(0); // Switch to Settings tab
	}
}

void JavaPage::refreshInstalledJavas()
{
	ui->installedJavaTree->clear();

	QString javaBaseDir = JavaUtils::managedJavaRoot();
	QDir baseDir(javaBaseDir);
	if (!baseDir.exists())
		return;

	// Scan for java binaries under java/{vendor}/{version}/
	QDirIterator vendorIt(javaBaseDir, QDir::Dirs | QDir::NoDotAndDotDot);
	while (vendorIt.hasNext()) {
		vendorIt.next();
		QString vendorName = vendorIt.fileName();
		QString vendorPath = vendorIt.filePath();

		QDirIterator versionIt(vendorPath, QDir::Dirs | QDir::NoDotAndDotDot);
		while (versionIt.hasNext()) {
			versionIt.next();
			QString versionPath = versionIt.filePath();

			// Look for java binary
#if defined(Q_OS_WIN)
			QString binaryName = "javaw.exe";
#else
			QString binaryName = "java";
#endif
			QDirIterator binIt(versionPath, QStringList() << binaryName,
							   QDir::Files, QDirIterator::Subdirectories);
			while (binIt.hasNext()) {
				binIt.next();
				QString javaPath = binIt.filePath();
				if (javaPath.contains("/bin/")) {
					auto* item = new QTreeWidgetItem(ui->installedJavaTree);
					item->setText(0, versionIt.fileName());
					item->setText(1, vendorName);
					item->setText(2, javaPath);
					break; // Only first binary per version dir
				}
			}
		}
	}

	ui->installedJavaTree->resizeColumnToContents(0);
	ui->installedJavaTree->resizeColumnToContents(1);
}
