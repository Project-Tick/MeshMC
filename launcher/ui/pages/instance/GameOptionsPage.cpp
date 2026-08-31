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

#include "GameOptionsPage.h"
#include "ui_GameOptionsPage.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/gameoptions/GameOptions.h"

GameOptionsPage::GameOptionsPage(MinecraftInstance* inst, QWidget* parent)
	: QWidget(parent), ui(new Ui::GameOptionsPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	m_model = inst->gameOptionsModel();
	ui->optionsView->setModel(m_model.get());
	auto head = ui->optionsView->header();
	if (head->count()) {
		head->setSectionResizeMode(0, QHeaderView::ResizeToContents);
		for (int i = 1; i < head->count(); i++) {
			head->setSectionResizeMode(i, QHeaderView::Stretch);
		}
	}
}

GameOptionsPage::~GameOptionsPage()
{
	// m_model->save();
}

void GameOptionsPage::openedImpl()
{
	// m_model->observe();
}

void GameOptionsPage::closedImpl()
{
	// m_model->unobserve();
}
