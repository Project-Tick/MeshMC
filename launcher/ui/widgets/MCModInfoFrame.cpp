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

#include "MCModInfoFrame.h"
#include "ui_MCModInfoFrame.h"

#include "ui/dialogs/CustomMessageBox.h"

void MCModInfoFrame::updateWithMod(Mod& m)
{
	if (m.type() == m.MOD_FOLDER) {
		clear();
		return;
	}

	QString text = "";
	QString name = "";
	if (m.name().isEmpty())
		name = m.mmc_id();
	else
		name = m.name();

	if (m.homeurl().isEmpty())
		text = name;
	else
		text = "<a href=\"" + m.homeurl() + "\">" + name + "</a>";
	if (!m.authors().isEmpty())
		text += " by " + m.authors().join(", ");

	setModText(text);

	if (m.description().isEmpty()) {
		setModDescription(QString());
	} else {
		setModDescription(m.description());
	}
}

void MCModInfoFrame::clear()
{
	setModText(QString());
	setModDescription(QString());
}

MCModInfoFrame::MCModInfoFrame(QWidget* parent)
	: QFrame(parent), ui(new Ui::MCModInfoFrame)
{
	ui->setupUi(this);
	ui->label_ModDescription->setHidden(true);
	ui->label_ModText->setHidden(true);
	updateHiddenState();
}

MCModInfoFrame::~MCModInfoFrame()
{
	delete ui;
}

void MCModInfoFrame::updateHiddenState()
{
	if (ui->label_ModDescription->isHidden() && ui->label_ModText->isHidden()) {
		setHidden(true);
	} else {
		setHidden(false);
	}
}

void MCModInfoFrame::setModText(QString text)
{
	if (text.isEmpty()) {
		ui->label_ModText->setHidden(true);
	} else {
		ui->label_ModText->setText(text);
		ui->label_ModText->setHidden(false);
	}
	updateHiddenState();
}

void MCModInfoFrame::setModDescription(QString text)
{
	if (text.isEmpty()) {
		ui->label_ModDescription->setHidden(true);
		updateHiddenState();
		return;
	} else {
		ui->label_ModDescription->setHidden(false);
		updateHiddenState();
	}
	ui->label_ModDescription->setToolTip("");
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
		ui->label_ModDescription->setOpenExternalLinks(false);
		ui->label_ModDescription->setTextFormat(Qt::TextFormat::RichText);
		desc = text;
		// This allows injecting HTML here.
		labeltext.append("<html><body>" + finaltext.left(287) +
						 "<a href=\"#mod_desc\">...</a></body></html>");
		QObject::connect(ui->label_ModDescription, &QLabel::linkActivated, this,
						 &MCModInfoFrame::modDescEllipsisHandler);
	} else {
		ui->label_ModDescription->setTextFormat(Qt::TextFormat::PlainText);
		labeltext.append(finaltext);
	}
	ui->label_ModDescription->setText(labeltext);
}

void MCModInfoFrame::modDescEllipsisHandler(const QString& link)
{
	if (!currentBox) {
		currentBox = CustomMessageBox::selectable(this, QString(), desc);
		connect(currentBox, &QMessageBox::finished, this,
				&MCModInfoFrame::boxClosed);
		currentBox->show();
	} else {
		currentBox->setText(desc);
	}
}

void MCModInfoFrame::boxClosed(int result)
{
	currentBox = nullptr;
}
