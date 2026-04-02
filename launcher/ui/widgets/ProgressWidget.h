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
 */// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QWidget>
#include <memory>

class Task;
class QProgressBar;
class QLabel;

class ProgressWidget : public QWidget
{
	Q_OBJECT
  public:
	explicit ProgressWidget(QWidget* parent = nullptr);

  public slots:
	void start(std::shared_ptr<Task> task);
	bool exec(std::shared_ptr<Task> task);

  private slots:
	void handleTaskFinish();
	void handleTaskStatus(const QString& status);
	void handleTaskProgress(qint64 current, qint64 total);
	void taskDestroyed();

  private:
	QLabel* m_label;
	QProgressBar* m_bar;
	std::shared_ptr<Task> m_task;
};
