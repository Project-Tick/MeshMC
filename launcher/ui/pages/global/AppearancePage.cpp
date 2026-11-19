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

#include "AppearancePage.h"
#include "ui_AppearancePage.h"

#include "Application.h"
#include "DesktopServices.h"
#include "ui/themes/AppearanceManager.h"
#include "ui/themes/ColorSchemes.h"
#include "ui/themes/ThemedIcons.h"

#include <QAction>
#include <QDir>
#include <QSignalBlocker>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>

#include <KColorSchemeManager>
#include <KColorSchemeModel>
#include <KStyleManager>

static const QStringList previewIconNames = {
	"new",	"centralmods", "viewfolder", "launch",
	"copy", "about",	   "settings",	 "accounts"};

AppearancePage::AppearancePage(QWidget* parent)
	: QWidget(parent), ui(new Ui::AppearancePage)
{
	ui->setupUi(this);

	// The colour schemes KDE Frameworks has, and the ones in the user's
	// "color-schemes" folder. KColorSchemeManager applies the one that is
	// chosen right away and remembers it, so there is nothing to save here.
	auto* schemes = KColorSchemeManager::instance();
	ui->colorSchemeComboBox->setModel(schemes->model());
	ui->colorSchemeComboBox->setCurrentIndex(
		schemes->indexForSchemeId(schemes->activeSchemeId()).row());
	connect(ui->colorSchemeComboBox, &QComboBox::activated, this,
			[schemes](int index) {
				schemes->activateSchemeId(
					schemes->model()
						->index(index, 0)
						.data(KColorSchemeModel::IdRole)
						.toString());
			});

	connect(ui->colorSchemeFolderButton, &QPushButton::clicked, this, [] {
		QDir().mkpath(ColorSchemes::userDirectory());
		DesktopServices::openDirectory(ColorSchemes::userDirectory(), true);
	});
	connect(ui->colorSchemeImportButton, &QPushButton::clicked, this,
			&AppearancePage::importColorSchemes);

	// The widget style: KStyleManager has the menu for it, which is not there
	// on KDE's platform theme, where the style is the desktop's to choose.
	QAction* style = KStyleManager::createConfigureAction(this);
	if (style && style->isVisible() && style->menu()) {
		ui->styleButton->setMenu(style->menu());
		ui->styleButton->setText(i18n("Choose…"));
	} else {
		ui->styleLabel->hide();
		ui->styleButton->hide();
	}

	// The icon theme: Breeze, and the ones in the user's icons folder.
	connect(ui->iconsComboBox, &QComboBox::activated, this, [this](int index) {
		APPLICATION->appearance()->setIconTheme(
			ui->iconsComboBox->itemData(index).toString());
	});
	connect(ui->iconsFolderButton, &QPushButton::clicked, this, [] {
		QDir().mkpath(ThemedIcons::userDirectory());
		DesktopServices::openDirectory(ThemedIcons::userDirectory(), true);
	});
	connect(ui->iconsReloadButton, &QPushButton::clicked, this,
			&AppearancePage::loadIconThemes);
	loadIconThemes();

	updateIconPreview();
	connect(APPLICATION->appearance(), &AppearanceManager::iconsChanged, this,
			&AppearancePage::updateIconPreview);
}

AppearancePage::~AppearancePage()
{
	delete ui;
}

bool AppearancePage::apply()
{
	// Nothing to apply: a colour scheme and a style take effect, and are
	// remembered, the moment they are chosen.
	return true;
}

void AppearancePage::loadIconThemes()
{
	QSignalBlocker blocker(ui->iconsComboBox);
	ui->iconsComboBox->clear();
	const QString current = ThemedIcons::theme();
	int currentIndex = 0;
	for (const auto& theme : ThemedIcons::themes()) {
		if (theme.id == current) {
			currentIndex = ui->iconsComboBox->count();
		}
		ui->iconsComboBox->addItem(theme.name, theme.id);
	}
	ui->iconsComboBox->setCurrentIndex(currentIndex);
}

void AppearancePage::importColorSchemes()
{
	const QStringList files = QFileDialog::getOpenFileNames(
		this, i18n("Import color schemes"), QString(),
		i18n("KDE color schemes (*.colors)"));
	if (files.isEmpty()) {
		return;
	}

	int added = 0;
	QStringList failed;
	for (const QString& file : files) {
		QString error;
		if (ColorSchemes::importScheme(file, ColorSchemes::userDirectory(),
									   &error)) {
			++added;
		} else {
			failed << QStringLiteral("%1: %2").arg(file, error);
		}
	}

	if (!failed.isEmpty()) {
		QMessageBox::warning(this, i18n("Import color schemes"),
							 i18n("Could not import:\n%1",
								  failed.join(QLatin1Char('\n'))));
	}
	if (added > 0) {
		QMessageBox::information(
			this, i18n("Import color schemes"),
			i18np("One color scheme was added. It is listed the next time "
				  "MeshMC starts.",
				  "%1 color schemes were added. They are listed the next "
				  "time MeshMC starts.",
				  added));
	}
}

void AppearancePage::updateIconPreview()
{
	QList<QToolButton*> previewButtons = {ui->icon1, ui->icon2, ui->icon3,
										  ui->icon4, ui->icon5, ui->icon6,
										  ui->icon7, ui->icon8};

	for (int i = 0; i < previewButtons.size() && i < previewIconNames.size();
		 i++) {
		previewButtons[i]->setIcon(
			APPLICATION->getThemedIcon(previewIconNames[i]));
	}
}
