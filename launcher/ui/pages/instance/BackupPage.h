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
