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

#include "NotificationDialog.h"
#include "ui_NotificationDialog.h"

#include <QTimerEvent>
#include <QStyle>

NotificationDialog::NotificationDialog(
	const NotificationChecker::NotificationEntry& entry, QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
						  Qt::WindowCloseButtonHint),
	  ui(new Ui::NotificationDialog)
{
	ui->setupUi(this);

	QStyle::StandardPixmap icon;
	switch (entry.type) {
		case NotificationChecker::NotificationEntry::Critical:
			icon = QStyle::SP_MessageBoxCritical;
			break;
		case NotificationChecker::NotificationEntry::Warning:
			icon = QStyle::SP_MessageBoxWarning;
			break;
		default:
		case NotificationChecker::NotificationEntry::Information:
			icon = QStyle::SP_MessageBoxInformation;
			break;
	}
	ui->iconLabel->setPixmap(style()->standardPixmap(icon, 0, this));
	ui->messageLabel->setText(entry.message);

	m_dontShowAgainText = tr("Don't show again");
	m_closeText = tr("Close");

	ui->dontShowAgainBtn->setText(m_dontShowAgainText +
								  QString(" (%1)").arg(m_dontShowAgainTime));
	ui->closeBtn->setText(m_closeText + QString(" (%1)").arg(m_closeTime));

	startTimer(1000);
}

NotificationDialog::~NotificationDialog()
{
	delete ui;
}

void NotificationDialog::timerEvent(QTimerEvent* event)
{
	if (m_dontShowAgainTime > 0) {
		m_dontShowAgainTime--;
		if (m_dontShowAgainTime == 0) {
			ui->dontShowAgainBtn->setText(m_dontShowAgainText);
			ui->dontShowAgainBtn->setEnabled(true);
		} else {
			ui->dontShowAgainBtn->setText(
				m_dontShowAgainText +
				QString(" (%1)").arg(m_dontShowAgainTime));
		}
	}
	if (m_closeTime > 0) {
		m_closeTime--;
		if (m_closeTime == 0) {
			ui->closeBtn->setText(m_closeText);
			ui->closeBtn->setEnabled(true);
		} else {
			ui->closeBtn->setText(m_closeText +
								  QString(" (%1)").arg(m_closeTime));
		}
	}

	if (m_closeTime == 0 && m_dontShowAgainTime == 0) {
		killTimer(event->timerId());
	}
}

void NotificationDialog::on_dontShowAgainBtn_clicked()
{
	done(DontShowAgain);
}
void NotificationDialog::on_closeBtn_clicked()
{
	done(Normal);
}
