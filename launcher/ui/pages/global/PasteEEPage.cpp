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

#include "PasteEEPage.h"
#include "ui_PasteEEPage.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTabBar>

#include "settings/SettingsObject.h"
#include "tools/BaseProfiler.h"
#include "Application.h"
#include "ui/widgets/ChoiceBinding.h"

PasteEEPage::PasteEEPage(QWidget* parent)
	: QWidget(parent), ui(new Ui::PasteEEPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	connect(ui->customAPIkeyEdit, &QLineEdit::textEdited, this,
			&PasteEEPage::textEdited);

	// The key is the shared one, or one of the user's own: a choice of radio
	// buttons, a text in the settings. The settings dialog binds it, like the
	// widgets named kcfg_<ID>.
	auto* key = new ChoiceBinding("PasteEEAPIKey", this);
	key->addChoice(ui->meshmcButton, "meshmc");
	key->addTextChoice(ui->customButton, ui->customAPIkeyEdit);
}

PasteEEPage::~PasteEEPage()
{
	delete ui;
}

bool PasteEEPage::apply()
{
	// Nothing is left to do: the settings dialog has saved the key.
	return true;
}

void PasteEEPage::textEdited(const QString& text)
{
	ui->customButton->setChecked(true);
}
