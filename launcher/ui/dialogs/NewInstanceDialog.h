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

#pragma once

#include <QDialog>

#include "BaseVersion.h"
#include "ui/pages/BasePageProvider.h"
#include "InstanceTask.h"

namespace Ui
{
	class NewInstanceDialog;
}

class PageContainer;
class QDialogButtonBox;
class ImportPage;
class FlamePage;

class NewInstanceDialog : public QDialog, public BasePageProvider
{
	Q_OBJECT

  public:
	explicit NewInstanceDialog(const QString& initialGroup,
							   const QString& url = QString(),
							   QWidget* parent = 0);
	~NewInstanceDialog();

	void updateDialogState();

	void setSuggestedPack(const QString& name = QString(),
						  InstanceTask* task = nullptr);
	void setSuggestedIconFromFile(const QString& path, const QString& name);
	void setSuggestedIcon(const QString& key);

	InstanceTask* extractTask();

	QString dialogTitle() override;
	QList<BasePage*> getPages() override;

	QString instName() const;
	QString instGroup() const;
	QString iconKey() const;

	/// The instance folder the user picked, as an absolute path.
	QString instDir() const;

  public slots:
	void accept() override;
	void reject() override;

  private slots:
	void on_iconButton_clicked();
	void on_instNameTextBox_textChanged(const QString& arg1);

  private:
	Ui::NewInstanceDialog* ui = nullptr;
	PageContainer* m_container = nullptr;
	QDialogButtonBox* m_buttons = nullptr;

	QString InstIconKey;
	ImportPage* importPage = nullptr;
	FlamePage* flamePage = nullptr;
	std::unique_ptr<InstanceTask> creationTask;

	bool importIcon = false;
	QString importIconPath;
	QString importIconName;

	void importIconNow();

	/*!
	 * Fill the folder box from the instance list's configured folders.
	 *
	 * Sourced from InstanceList rather than from the settings directly:
	 * that list has already been resolved, deduplicated, and reduced to
	 * the folders that actually exist, so a folder the user typed but that
	 * could not be created cannot be offered here as a destination.
	 */
	void refreshInstDirBox();
};
