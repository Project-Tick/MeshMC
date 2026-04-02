/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include <QMainWindow>

#include "ui/pages/BasePage.h"
#include <Application.h>

class QFileSystemModel;
class QIdentityProxyModel;
namespace Ui
{
	class ScreenshotsPage;
}

struct ScreenShot;
class ScreenshotList;
class ImgurAlbumCreation;

class ScreenshotsPage : public QMainWindow, public BasePage
{
	Q_OBJECT

  public:
	explicit ScreenshotsPage(QString path, QWidget* parent = 0);
	virtual ~ScreenshotsPage();

	virtual void openedImpl() override;

	enum { NothingDone = 0x42 };

	virtual bool eventFilter(QObject*, QEvent*) override;
	virtual QString displayName() const override
	{
		return tr("Screenshots");
	}
	virtual QIcon icon() const override
	{
		return APPLICATION->getThemedIcon("screenshots");
	}
	virtual QString id() const override
	{
		return "screenshots";
	}
	virtual QString helpPage() const override
	{
		return "Screenshots-management";
	}
	virtual bool apply() override
	{
		return !m_uploadActive;
	}

  protected:
	QMenu* createPopupMenu() override;

  private slots:
	void on_actionUpload_triggered();
	void on_actionDelete_triggered();
	void on_actionRename_triggered();
	void on_actionView_Folder_triggered();
	void onItemActivated(QModelIndex);
	void ShowContextMenu(const QPoint& pos);

  private:
	Ui::ScreenshotsPage* ui;
	std::shared_ptr<QFileSystemModel> m_model;
	std::shared_ptr<QIdentityProxyModel> m_filterModel;
	QString m_folder;
	bool m_valid = false;
	bool m_uploadActive = false;
};
