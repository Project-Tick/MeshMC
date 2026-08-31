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

#include <QToolBar>
#include <QAction>
#include <QMap>

class QMenu;

class WideBar : public QToolBar
{
	Q_OBJECT

  public:
	explicit WideBar(const QString& title, QWidget* parent = nullptr);
	explicit WideBar(QWidget* parent = nullptr);
	virtual ~WideBar();

	void addAction(QAction* action);
	void addSeparator();
	void insertSpacer(QAction* action);
	QMenu* createContextMenu(QWidget* parent = nullptr,
							 const QString& title = QString());

	/* Re-read the actions into the buttons that stand for them.
	 *
	 * The buttons copy what an action looks like when they are built,
	 * which for a bar coming out of a .ui file is before the page's
	 * constructor has had a chance to say anything. A page that gives an
	 * action a menu at that point needs to say so, because the button is
	 * already there and QAction has no way for us to ask it to announce
	 * itself again. */
	void refreshActions();

  private:
	struct BarEntry;
	QList<BarEntry*> m_entries;
};
