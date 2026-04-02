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

#include "AppearancePage.h"
#include "ui_AppearancePage.h"

#include "Application.h"
#include "ui/themes/ITheme.h"
#include "ui/themes/ThemeManager.h"
#include "ui/themes/CatPack.h"

#include <QGraphicsOpacityEffect>

static const QStringList previewIconNames = {
	"new",	"centralmods", "viewfolder", "launch",
	"copy", "about",	   "settings",	 "accounts"};

AppearancePage::AppearancePage(QWidget* parent)
	: QWidget(parent), ui(new Ui::AppearancePage)
{
	ui->setupUi(this);

	ui->catPreview->setGraphicsEffect(new QGraphicsOpacityEffect(this));

	connect(ui->widgetStyleComboBox,
			QOverload<int>::of(&QComboBox::currentIndexChanged), this,
			&AppearancePage::applyWidgetTheme);
	connect(ui->iconsComboBox,
			QOverload<int>::of(&QComboBox::currentIndexChanged), this,
			&AppearancePage::applyIconTheme);
	connect(ui->catPackComboBox,
			QOverload<int>::of(&QComboBox::currentIndexChanged), this,
			&AppearancePage::applyCatTheme);

	loadSettings();
}

AppearancePage::~AppearancePage()
{
	delete ui;
}

bool AppearancePage::apply()
{
	applySettings();
	return true;
}

void AppearancePage::applyWidgetTheme(int index)
{
	auto settings = APPLICATION->settings();
	auto originalTheme = settings->get("ApplicationTheme").toString();
	auto newTheme = ui->widgetStyleComboBox->itemData(index).toString();
	if (originalTheme != newTheme) {
		settings->set("ApplicationTheme", newTheme);
		APPLICATION->themeManager()->applyCurrentlySelectedTheme();

		// Sync icon combo to the auto-resolved icon theme
		auto resolvedIcon = settings->get("IconTheme").toString();
		ui->iconsComboBox->blockSignals(true);
		for (int i = 0; i < ui->iconsComboBox->count(); i++) {
			if (ui->iconsComboBox->itemData(i).toString() == resolvedIcon) {
				ui->iconsComboBox->setCurrentIndex(i);
				break;
			}
		}
		ui->iconsComboBox->blockSignals(false);
	}
	updateIconPreview();
}

void AppearancePage::applyIconTheme(int index)
{
	auto settings = APPLICATION->settings();
	auto originalIconTheme = settings->get("IconTheme").toString();
	auto newIconTheme = ui->iconsComboBox->itemData(index).toString();
	if (originalIconTheme != newIconTheme) {
		settings->set("IconTheme", newIconTheme);
		APPLICATION->themeManager()->applyCurrentlySelectedTheme();
	}
	updateIconPreview();
}

void AppearancePage::applySettings()
{
	// Theme and icon changes are already persisted live via
	// applyWidgetTheme/applyIconTheme. This is intentionally minimal — settings
	// are saved on combo change.
}

void AppearancePage::loadSettings()
{
	auto settings = APPLICATION->settings();
	auto tm = APPLICATION->themeManager();

	// Block signals during population
	ui->widgetStyleComboBox->blockSignals(true);
	ui->iconsComboBox->blockSignals(true);
	ui->catPackComboBox->blockSignals(true);

	// --- Widget themes (flat list) ---
	ui->widgetStyleComboBox->clear();
	auto currentThemeId = settings->get("ApplicationTheme").toString();
	auto themes = tm->allThemes();
	int themeIdx = 0;

	for (size_t i = 0; i < themes.size(); i++) {
		auto* theme = themes[i];
		ui->widgetStyleComboBox->addItem(theme->name(), theme->id());
		if (!theme->tooltip().isEmpty()) {
			ui->widgetStyleComboBox->setItemData(
				static_cast<int>(i), theme->tooltip(), Qt::ToolTipRole);
		}
		if (theme->id() == currentThemeId) {
			themeIdx = static_cast<int>(i);
		}
	}

	ui->widgetStyleComboBox->setCurrentIndex(themeIdx);

	// --- Icon themes (flat list) ---
	ui->iconsComboBox->clear();
	auto currentIconTheme = settings->get("IconTheme").toString();
	auto iconThemeList = tm->iconThemes();
	int iconIdx = 0;

	for (int i = 0; i < iconThemeList.size(); i++) {
		const auto& entry = iconThemeList[i];
		ui->iconsComboBox->addItem(entry.name, entry.id);
		if (entry.id == currentIconTheme) {
			iconIdx = i;
		}
	}

	ui->iconsComboBox->setCurrentIndex(iconIdx);

	// --- Cat Packs ---
	ui->catPackComboBox->clear();
	auto currentCat = settings->get("BackgroundCat").toString();
	auto cats = tm->getValidCatPacks();
	int catIdx = 0;

	for (int i = 0; i < cats.size(); i++) {
		auto* cat = cats[i];
		QIcon catIcon(cat->path());
		ui->catPackComboBox->addItem(catIcon, cat->name(), cat->id());
		if (cat->id() == currentCat) {
			catIdx = i;
		}
	}

	ui->catPackComboBox->setCurrentIndex(catIdx);

	// Unblock signals
	ui->widgetStyleComboBox->blockSignals(false);
	ui->iconsComboBox->blockSignals(false);
	ui->catPackComboBox->blockSignals(false);

	// Initial previews
	updateIconPreview();
	updateCatPreview();
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

void AppearancePage::applyCatTheme(int index)
{
	auto settings = APPLICATION->settings();
	auto originalCat = settings->get("BackgroundCat").toString();
	auto newCat = ui->catPackComboBox->itemData(index).toString();
	if (originalCat != newCat) {
		settings->set("BackgroundCat", newCat);
	}
	updateCatPreview();
}

void AppearancePage::updateCatPreview()
{
	QIcon catPackIcon(APPLICATION->themeManager()->getCatPack());
	ui->catPreview->setIcon(catPackIcon);

	auto effect =
		dynamic_cast<QGraphicsOpacityEffect*>(ui->catPreview->graphicsEffect());
	if (effect)
		effect->setOpacity(1.0);
}
