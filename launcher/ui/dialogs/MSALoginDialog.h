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

#include <QtWidgets/QDialog>
#include <QtCore/QEventLoop>

#include "minecraft/auth/MinecraftAccount.h"

namespace Ui
{
	class MSALoginDialog;
}

class MSALoginDialog : public QDialog
{
	Q_OBJECT

  public:
	~MSALoginDialog();

	static MinecraftAccountPtr newAccount(QWidget* parent, QString message);
	int exec() override;

  private:
	explicit MSALoginDialog(QWidget* parent = 0);

	void setUserInputsEnabled(bool enable);

  protected slots:
	void onTaskFailed(const QString& reason);
	void onTaskSucceeded();
	void onTaskStatus(const QString& status);
	void onTaskProgress(qint64 current, qint64 total);
	void onAuthorizeWithBrowser(const QUrl& url);

  private:
	Ui::MSALoginDialog* ui;
	MinecraftAccountPtr m_account;
	shared_qobject_ptr<AccountTask> m_loginTask;
};
