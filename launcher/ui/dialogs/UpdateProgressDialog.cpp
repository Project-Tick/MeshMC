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

#include "UpdateProgressDialog.h"

#include <QVBoxLayout>

UpdateProgressDialog::UpdateProgressDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle(tr("MeshMC Update"));
	setMinimumSize(500, 400);
	setModal(true);

	auto* layout = new QVBoxLayout(this);

	m_statusLabel = new QLabel(tr("Checking for updates..."), this);
	m_statusLabel->setAlignment(Qt::AlignCenter);
	QFont font = m_statusLabel->font();
	font.setPointSize(12);
	font.setBold(true);
	m_statusLabel->setFont(font);
	layout->addWidget(m_statusLabel);

	m_progressBar = new QProgressBar(this);
	m_progressBar->setRange(0, 0); // indeterminate by default
	layout->addWidget(m_progressBar);

	m_logView = new QPlainTextEdit(this);
	m_logView->setReadOnly(true);
	m_logView->setMaximumBlockCount(1000);
	QString fontFamily = "monospace";
	m_logView->document()->setDefaultFont(QFont(fontFamily, 10));
	layout->addWidget(m_logView);

	m_closeButton = new QPushButton(tr("Cancel"), this);
	connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
	layout->addWidget(m_closeButton);
}

void UpdateProgressDialog::setStatus(const QString& status)
{
	m_statusLabel->setText(status);
	appendLog(status);
}

void UpdateProgressDialog::appendLog(const QString& line)
{
	m_logView->appendPlainText(line);
}

void UpdateProgressDialog::setProgress(int value, int maximum)
{
	m_progressBar->setRange(0, maximum);
	m_progressBar->setValue(value);
}

void UpdateProgressDialog::setFinished(bool success, const QString& message)
{
	m_statusLabel->setText(message);
	m_progressBar->setRange(0, 100);
	m_progressBar->setValue(success ? 100 : 0);
	m_closeButton->setText(tr("Close"));
	appendLog(message);
}
