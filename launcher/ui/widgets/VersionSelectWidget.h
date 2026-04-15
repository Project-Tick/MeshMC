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

#include <QWidget>
#include <QSortFilterProxyModel>
#include "BaseVersionList.h"

class VersionProxyModel;
class VersionListView;
class QVBoxLayout;
class QProgressBar;
class Filter;

class VersionSelectWidget : public QWidget
{
	Q_OBJECT
  public:
	explicit VersionSelectWidget(QWidget* parent = 0);
	~VersionSelectWidget();

	//! loads the list if needed.
	void initialize(BaseVersionList* vlist);

	//! Starts a task that loads the list.
	void loadList();

	bool hasVersions() const;
	BaseVersionPtr selectedVersion() const;
	void selectRecommended();
	void selectCurrent();

	void setCurrentVersion(const QString& version);
	void setFuzzyFilter(BaseVersionList::ModelRoles role, QString filter);
	void setExactFilter(BaseVersionList::ModelRoles role, QString filter);
	void setFilter(BaseVersionList::ModelRoles role, Filter* filter);
	void setEmptyString(QString emptyString);
	void setEmptyErrorString(QString emptyErrorString);
	void setResizeOn(int column);

  signals:
	void selectedVersionChanged(BaseVersionPtr version);

  protected:
	virtual void closeEvent(QCloseEvent*);

  private slots:
	void onTaskSucceeded();
	void onTaskFailed(const QString& reason);
	void changeProgress(qint64 current, qint64 total);
	void currentRowChanged(const QModelIndex& current, const QModelIndex&);

  private:
	void preselect();

  private:
	QString m_currentVersion;
	BaseVersionList* m_vlist = nullptr;
	VersionProxyModel* m_proxyModel = nullptr;
	int resizeOnColumn = 0;
	Task* loadTask;
	bool preselectedAlready = false;

  private:
	QVBoxLayout* verticalLayout = nullptr;
	VersionListView* listView = nullptr;
	QProgressBar* sneakyProgressBar = nullptr;
};
