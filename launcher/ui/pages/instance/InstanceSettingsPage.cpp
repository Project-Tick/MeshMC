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

#include "InstanceSettingsPage.h"
#include "ui_InstanceSettingsPage.h"

#include <QFileDialog>
#include <QDialog>
#include <QMessageBox>

#include "Sys.h"

#include "ui/dialogs/VersionSelectDialog.h"
#include "ui/widgets/CustomCommands.h"
#include "ui/widgets/OverrideGroup.h"

#include "JavaCommon.h"
#include "Application.h"

#include "java/JavaInstallList.h"
#include "java/download/JavaRuntime.h"
#include "FileSystem.h"
#include <KLocalizedString>

InstanceSettingsPage::InstanceSettingsPage(BaseInstance* inst, QWidget* parent)
	: QWidget(parent), ui(new Ui::InstanceSettingsPage), m_instance(inst)
{
	m_settings = inst->settings();
	ui->setupUi(this);
	auto sysMB = Sys::getSystemRam() / Sys::mebibyte;
	ui->maxMemSpinBox->setMaximum(sysMB);

	// Populate vendor combobox
	auto providers = JavaDownload::JavaProviderInfo::availableProviders();
	for (const auto& provider : providers) {
		ui->javaAutoDownloadVendorBox->addItem(provider.name, provider.uid);
	}

	setupGroups();

	connect(ui->openGlobalJavaSettingsButton, &QCommandLinkButton::clicked,
			this, &InstanceSettingsPage::globalSettingsButtonClicked);
	connect(APPLICATION, &Application::globalSettingsAboutToOpen, this,
			&InstanceSettingsPage::applySettings);
	connect(APPLICATION, &Application::globalSettingsClosed, this,
			&InstanceSettingsPage::loadSettings);
	emit APPLICATION->instanceSettingsPageCreated(this, m_instance);
	loadSettings();
}

void InstanceSettingsPage::setupGroups()
{
	// Each group is a gate setting of the instance, the widgets that show the
	// settings it opens up, and what they are called.
	auto add = [this](OverrideGroup* group) {
		m_groups.append(group);
		return group;
	};

	add(new OverrideGroup(m_settings.get(), ui->consoleSettingsBox,
						  "OverrideConsole"))
		->bind("ShowConsole", ui->showConsoleCheck)
		.bind("AutoCloseConsole", ui->autoCloseConsoleCheck)
		.bind("ShowConsoleOnError", ui->showConsoleErrorCheck);

	add(new OverrideGroup(m_settings.get(), ui->windowSizeGroupBox,
						  "OverrideWindow"))
		->bind("LaunchMaximized", ui->maximizedCheckBox)
		.bind("MinecraftWinWidth", ui->windowWidthSpinBox)
		.bind("MinecraftWinHeight", ui->windowHeightSpinBox);

	m_memory = add(new OverrideGroup(m_settings.get(), ui->memoryGroupBox,
									 "OverrideMemory"));
	m_memory->bind("MinMemAlloc", ui->minMemSpinBox)
		.bind("MaxMemAlloc", ui->maxMemSpinBox)
		.bind("PermGen", ui->permGenSpinBox);

	// "OverrideJava" was one gate for the location and the arguments, before
	// there were two.
	add(new OverrideGroup(m_settings.get(), ui->javaSettingsGroupBox,
						  "OverrideJavaLocation"))
		->legacyGate("OverrideJava")
		.bind("JavaPath", ui->javaPathTextBox);

	m_javaArgs = add(new OverrideGroup(m_settings.get(),
									   ui->javaArgumentsGroupBox,
									   "OverrideJavaArgs"));
	m_javaArgs->legacyGate("OverrideJava")
		.bind("JvmArgs", ui->jvmArgsTextBox)
		// it is a line in the settings
		.transformOnApply("JvmArgs", [](QVariant text) {
			return QVariant(text.toString().replace("\n", " "));
		});

	add(new OverrideGroup(m_settings.get(), ui->javaVendorGroupBox,
						  "OverrideJavaAutoDownloadVendor"))
		->bind("JavaAutoDownloadVendor", ui->javaAutoDownloadVendorBox);

	// CustomCommands is a widget of its own: its group box is not one of the
	// page's, and it reads and writes the three commands as a whole.
	add(new OverrideGroup(
		   m_settings.get(), "OverrideCommands",
		   [this] { return ui->customCommands->checked(); },
		   [this](bool on) { ui->customCommands->setChecked(on); }))
		->bind(
			"PreLaunchCommand",
			[this] { return ui->customCommands->prelaunchCommand(); },
			[this](const QVariant& v) {
				ui->customCommands->setPrelaunchCommand(v.toString());
			})
		.bind(
			"WrapperCommand",
			[this] { return ui->customCommands->wrapperCommand(); },
			[this](const QVariant& v) {
				ui->customCommands->setWrapperCommand(v.toString());
			})
		.bind(
			"PostExitCommand",
			[this] { return ui->customCommands->postexitCommand(); },
			[this](const QVariant& v) {
				ui->customCommands->setPostexitCommand(v.toString());
			});

	add(new OverrideGroup(m_settings.get(), ui->nativeWorkaroundsGroupBox,
						  "OverrideNativeWorkarounds"))
		->bind("UseNativeOpenAL", ui->useNativeOpenALCheck)
		.bind("UseNativeGLFW", ui->useNativeGLFWCheck);

	add(new OverrideGroup(m_settings.get(), ui->gameTimeGroupBox,
						  "OverrideGameTime"))
		->bind("ShowGameTime", ui->showGameTime)
		.bind("RecordGameTime", ui->recordGameTime);

	// These two are the instance's own, with no global setting under them:
	// the gate is the setting.
	add(new OverrideGroup(m_settings.get(), ui->serverJoinGroupBox,
						  "JoinServerOnLaunch"))
		->bind("JoinServerOnLaunchAddress", ui->serverJoinAddress);

	add(new OverrideGroup(m_settings.get(), ui->globalDataPacksGroupBox,
						  "GlobalDataPacksEnabled"))
		->bind("GlobalDataPacksPath", ui->globalDataPacksPath)
		.transformOnApply("GlobalDataPacksPath", [](QVariant path) {
			return QVariant(path.toString().trimmed());
		});
}

bool InstanceSettingsPage::shouldDisplay() const
{
	return !m_instance->isRunning();
}

InstanceSettingsPage::~InstanceSettingsPage()
{
	qDeleteAll(m_groups);
	delete ui;
}

void InstanceSettingsPage::globalSettingsButtonClicked(bool)
{
	switch (ui->settingsTabs->currentIndex()) {
		case 0:
			APPLICATION->ShowGlobalSettings(this, "java-settings");
			return;
		case 1:
			APPLICATION->ShowGlobalSettings(this, "minecraft-settings");
			return;
		case 2:
			APPLICATION->ShowGlobalSettings(this, "custom-commands");
			return;
	}
}

bool InstanceSettingsPage::apply()
{
	applySettings();
	return true;
}

void InstanceSettingsPage::applySettings()
{
	SettingsObject::Lock lock(m_settings);

	// What the groups write and reset is in OverrideGroup. What is left here
	// is what a group cannot say by itself.

	// The smaller of the two memory values is the minimum.
	if (m_memory->isOn()) {
		int min = ui->minMemSpinBox->value();
		int max = ui->maxMemSpinBox->value();
		if (min > max) {
			ui->minMemSpinBox->setValue(max);
			ui->maxMemSpinBox->setValue(min);
		}
	}

	for (OverrideGroup* group : m_groups) {
		group->apply();
	}

	// The arguments are checked.
	if (m_javaArgs->isOn()) {
		JavaCommon::checkJVMArgs(m_settings->get("JvmArgs").toString(),
								 this->parentWidget());
	}

	// old generic 'override both' is removed.
	m_settings->reset("OverrideJava");

	emit settingsAboutToApply();
}

void InstanceSettingsPage::loadSettings()
{
	for (OverrideGroup* group : m_groups) {
		group->load();
	}

	// With a smaller maximum than minimum, the two are swapped.
	{
		int min = ui->minMemSpinBox->value();
		int max = ui->maxMemSpinBox->value();
		if (min > max) {
			ui->minMemSpinBox->setValue(max);
			ui->maxMemSpinBox->setValue(min);
		}
	}

	bool permGenVisible = m_settings->get("PermGenVisible").toBool();
	ui->permGenSpinBox->setVisible(permGenVisible);
	ui->labelPermGen->setVisible(permGenVisible);
	ui->labelPermgenNote->setVisible(permGenVisible);

	emit settingsLoaded();
}

void InstanceSettingsPage::on_javaDetectBtn_clicked()
{
	JavaInstallPtr java;

	VersionSelectDialog vselect(APPLICATION->javalist().get(),
								i18n("Select a Java version"), this, true);
	vselect.setResizeOn(2);
	vselect.exec();

	if (vselect.result() == QDialog::Accepted && vselect.selectedVersion()) {
		java =
			std::dynamic_pointer_cast<JavaInstall>(vselect.selectedVersion());
		ui->javaPathTextBox->setText(java->path);
		bool visible = java->id.requiresPermGen() &&
					   m_settings->get("OverrideMemory").toBool();
		ui->permGenSpinBox->setVisible(visible);
		ui->labelPermGen->setVisible(visible);
		ui->labelPermgenNote->setVisible(visible);
		m_settings->set("PermGenVisible", visible);
	}
}

void InstanceSettingsPage::on_javaBrowseBtn_clicked()
{
	QString raw_path =
		QFileDialog::getOpenFileName(this, i18n("Find Java executable"));

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if (raw_path.isEmpty()) {
		return;
	}
	QString cooked_path = FS::NormalizePath(raw_path);

	QFileInfo javaInfo(cooked_path);
	if (!javaInfo.exists() || !javaInfo.isExecutable()) {
		return;
	}
	ui->javaPathTextBox->setText(cooked_path);

	// custom Java could be anything... enable perm gen option
	ui->permGenSpinBox->setVisible(true);
	ui->labelPermGen->setVisible(true);
	ui->labelPermgenNote->setVisible(true);
	m_settings->set("PermGenVisible", true);
}

void InstanceSettingsPage::on_javaTestBtn_clicked()
{
	if (checker) {
		return;
	}
	checker.reset(new JavaCommon::TestCheck(
		this, ui->javaPathTextBox->text(),
		ui->jvmArgsTextBox->toPlainText().replace("\n", " "),
		ui->minMemSpinBox->value(), ui->maxMemSpinBox->value(),
		ui->permGenSpinBox->value()));
	connect(checker.get(), &JavaCommon::TestCheck::finished, this,
			&InstanceSettingsPage::checkerFinished);
	checker->run();
}

void InstanceSettingsPage::checkerFinished()
{
	checker.reset();
}
