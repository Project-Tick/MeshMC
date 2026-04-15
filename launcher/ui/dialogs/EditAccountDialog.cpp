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

#include "EditAccountDialog.h"
#include "ui_EditAccountDialog.h"
#include <DesktopServices.h>
#include <QUrl>

EditAccountDialog::EditAccountDialog(const QString& text, QWidget* parent,
									 int flags)
	: QDialog(parent), ui(new Ui::EditAccountDialog)
{
	ui->setupUi(this);

	ui->label->setText(text);
	ui->label->setVisible(!text.isEmpty());

	ui->userTextBox->setEnabled(flags & UsernameField);
	ui->passTextBox->setEnabled(flags & PasswordField);
}

EditAccountDialog::~EditAccountDialog()
{
	delete ui;
}

void EditAccountDialog::on_label_linkActivated(const QString& link)
{
	DesktopServices::openUrl(QUrl(link));
}

void EditAccountDialog::setUsername(const QString& user) const
{
	ui->userTextBox->setText(user);
}

QString EditAccountDialog::username() const
{
	return ui->userTextBox->text();
}

void EditAccountDialog::setPassword(const QString& pass) const
{
	ui->passTextBox->setText(pass);
}

QString EditAccountDialog::password() const
{
	return ui->passTextBox->text();
}
