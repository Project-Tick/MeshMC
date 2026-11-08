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

#include "MinecraftPage.h"
#include "ui_MinecraftPage.h"

#include <QMessageBox>
#include <QDir>
#include <QTabBar>

#include "settings/SettingsObject.h"
#include "Application.h"

MinecraftPage::MinecraftPage(QWidget* parent)
	: QWidget(parent), ui(new Ui::MinecraftPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	// The widgets named kcfg_<ID> are bound to the global settings by the
	// settings dialog (KConfigDialogManager); it sets them, and so also when
	// the dialog shows the defaults.
	connect(ui->kcfg_LaunchMaximized, &QCheckBox::toggled, this,
			&MinecraftPage::updateCheckboxStuff);
	updateCheckboxStuff();
}

MinecraftPage::~MinecraftPage()
{
	delete ui;
}

void MinecraftPage::updateCheckboxStuff()
{
	ui->kcfg_MinecraftWinWidth->setEnabled(!ui->kcfg_LaunchMaximized->isChecked());
	ui->kcfg_MinecraftWinHeight->setEnabled(!ui->kcfg_LaunchMaximized->isChecked());
}
