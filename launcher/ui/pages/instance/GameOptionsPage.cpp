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
