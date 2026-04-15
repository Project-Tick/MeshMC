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

#include "CustomCommands.h"
#include "ui_CustomCommands.h"

CustomCommands::~CustomCommands()
{
	delete ui;
}

CustomCommands::CustomCommands(QWidget* parent)
	: QWidget(parent), ui(new Ui::CustomCommands)
{
	ui->setupUi(this);
}

void CustomCommands::initialize(bool checkable, bool checked,
								const QString& prelaunch,
								const QString& wrapper, const QString& postexit)
{
	ui->customCommandsGroupBox->setCheckable(checkable);
	if (checkable) {
		ui->customCommandsGroupBox->setChecked(checked);
	}
	ui->preLaunchCmdTextBox->setText(prelaunch);
	ui->wrapperCmdTextBox->setText(wrapper);
	ui->postExitCmdTextBox->setText(postexit);
}

bool CustomCommands::checked() const
{
	if (!ui->customCommandsGroupBox->isCheckable())
		return true;
	return ui->customCommandsGroupBox->isChecked();
}

QString CustomCommands::prelaunchCommand() const
{
	return ui->preLaunchCmdTextBox->text();
}

QString CustomCommands::wrapperCommand() const
{
	return ui->wrapperCmdTextBox->text();
}

QString CustomCommands::postexitCommand() const
{
	return ui->postExitCmdTextBox->text();
}
