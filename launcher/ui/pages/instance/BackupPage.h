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

#pragma once

#include <QWidget>
#include <memory>

#include "BaseInstance.h"
#include "backup/BackupManager.h"
#include "ui/pages/BasePage.h"
#include <Application.h>

namespace Ui
{
	class BackupPage;
}

class BackupPage : public QWidget, public BasePage
{
	Q_OBJECT

  public:
	explicit BackupPage(BaseInstance* inst, QWidget* parent = nullptr);
	~BackupPage() override;

	QString id() const override
	{
		return "backup-system";
	}
	QString displayName() const override
	{
		return tr("Backups");
	}
	QIcon icon() const override
	{
		return APPLICATION->getThemedIcon("backup");
	}
	QString helpPage() const override
	{
		return "Instance-Backups";
	}

	void openedImpl() override;

  private slots:
	void on_btnCreate_clicked();
	void on_btnRestore_clicked();
	void on_btnExport_clicked();
	void on_btnImport_clicked();
	void on_btnDelete_clicked();
	void onSelectionChanged();

  private:
	void refreshList();
	void updateButtons();
	BackupEntry selectedEntry() const;
	static QString humanFileSize(qint64 bytes);

	Ui::BackupPage* ui;
	BaseInstance* m_inst;
	std::unique_ptr<BackupManager> m_manager;
	QList<BackupEntry> m_entries;
};
