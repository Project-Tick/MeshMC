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

#include <QMessageBox>
#include <QtGui>

#include "ErrorFrame.h"
#include "ui_ErrorFrame.h"

#include "ui/dialogs/CustomMessageBox.h"

void ErrorFrame::clear()
{
	setTitle(QString());
	setDescription(QString());
}

ErrorFrame::ErrorFrame(QWidget* parent) : QFrame(parent), ui(new Ui::ErrorFrame)
{
	ui->setupUi(this);
	ui->label_Description->setHidden(true);
	ui->label_Title->setHidden(true);
	updateHiddenState();
}

ErrorFrame::~ErrorFrame()
{
	delete ui;
}

void ErrorFrame::updateHiddenState()
{
	if (ui->label_Description->isHidden() && ui->label_Title->isHidden()) {
		setHidden(true);
	} else {
		setHidden(false);
	}
}

void ErrorFrame::setTitle(QString text)
{
	if (text.isEmpty()) {
		ui->label_Title->setHidden(true);
	} else {
		ui->label_Title->setText(text);
		ui->label_Title->setHidden(false);
	}
	updateHiddenState();
}

void ErrorFrame::setDescription(QString text)
{
	if (text.isEmpty()) {
		ui->label_Description->setHidden(true);
		updateHiddenState();
		return;
	} else {
		ui->label_Description->setHidden(false);
		updateHiddenState();
	}
	ui->label_Description->setToolTip("");
	QString intermediatetext = text.trimmed();
	bool prev(false);
	QChar rem('\n');
	QString finaltext;
	finaltext.reserve(intermediatetext.size());
	foreach (const QChar& c, intermediatetext) {
		if (c == rem && prev) {
			continue;
		}
		prev = c == rem;
		finaltext += c;
	}
	QString labeltext;
	labeltext.reserve(300);
	if (finaltext.length() > 290) {
		ui->label_Description->setOpenExternalLinks(false);
		ui->label_Description->setTextFormat(Qt::TextFormat::RichText);
		desc = text;
		// This allows injecting HTML here.
		labeltext.append("<html><body>" + finaltext.left(287) +
						 "<a href=\"#mod_desc\">...</a></body></html>");
		QObject::connect(ui->label_Description, &QLabel::linkActivated, this,
						 &ErrorFrame::ellipsisHandler);
	} else {
		ui->label_Description->setTextFormat(Qt::TextFormat::PlainText);
		labeltext.append(finaltext);
	}
	ui->label_Description->setText(labeltext);
}

void ErrorFrame::ellipsisHandler(const QString& link)
{
	if (!currentBox) {
		currentBox = CustomMessageBox::selectable(this, QString(), desc);
		connect(currentBox, &QMessageBox::finished, this,
				&ErrorFrame::boxClosed);
		currentBox->show();
	} else {
		currentBox->setText(desc);
	}
}

void ErrorFrame::boxClosed(int result)
{
	currentBox = nullptr;
}
