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
