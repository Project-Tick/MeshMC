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

#include "LegacyUpgradePage.h"
#include "ui_LegacyUpgradePage.h"

#include "InstanceList.h"
#include "minecraft/legacy/LegacyInstance.h"
#include "minecraft/legacy/LegacyUpgradeTask.h"
#include "Application.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"

LegacyUpgradePage::LegacyUpgradePage(InstancePtr inst, QWidget* parent)
	: QWidget(parent), ui(new Ui::LegacyUpgradePage), m_inst(inst)
{
	ui->setupUi(this);
}

LegacyUpgradePage::~LegacyUpgradePage()
{
	delete ui;
}

void LegacyUpgradePage::runModalTask(Task* task)
{
	connect(task, &Task::failed, [this](QString reason) {
		CustomMessageBox::selectable(this, tr("Error"), reason,
									 QMessageBox::Warning)
			->show();
	});
	ProgressDialog loadDialog(this);
	loadDialog.setSkipButton(true, tr("Abort"));
	if (loadDialog.execWithTask(task) == QDialog::Accepted) {
		m_container->requestClose();
	}
}

void LegacyUpgradePage::on_upgradeButton_clicked()
{
	QString newName = tr("%1 (Migrated)").arg(m_inst->name());
	auto upgradeTask = new LegacyUpgradeTask(m_inst);
	upgradeTask->setName(newName);
	upgradeTask->setGroup(
		APPLICATION->instances()->getInstanceGroup(m_inst->id()));
	upgradeTask->setIcon(m_inst->iconKey());
	unique_qobject_ptr<Task> task(
		APPLICATION->instances()->wrapInstanceTask(upgradeTask));
	runModalTask(task.get());
}

bool LegacyUpgradePage::shouldDisplay() const
{
	return !m_inst->isRunning();
}
