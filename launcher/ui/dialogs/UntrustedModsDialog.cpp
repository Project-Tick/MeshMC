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

#include "UntrustedModsDialog.h"
#include "ui_UntrustedModsDialog.h"

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTimer>

/* How long the confirmation stays out of reach.
 *
 * Long enough that the dialog has to be looked at, short enough not to
 * feel like a punishment for installing a pack from a friend. */
static constexpr int kConfirmDelayMs = 3000;

UntrustedModsDialog::UntrustedModsDialog(const QStringList& paths,
										 QWidget* parent)
	: QDialog(parent), m_ui(new Ui::UntrustedModsDialog)
{
	m_ui->setupUi(this);
	m_ui->modList->addItems(paths);

	auto* ok = m_ui->buttonBox->button(QDialogButtonBox::Ok);
	ok->setText(tr("Install anyway"));
	/* Unavailable until the box is ticked, and the box itself is
	 * unavailable at first, so the fastest possible "yes" still involves
	 * reading something. */
	ok->setEnabled(false);
	connect(m_ui->confirmCheckbox, &QAbstractButton::toggled, ok,
			&QWidget::setEnabled);

	m_ui->confirmCheckbox->setEnabled(false);
	QTimer::singleShot(kConfirmDelayMs, this,
					   [this] { m_ui->confirmCheckbox->setEnabled(true); });

	/* Cancel is what a stray Escape or Return should land on: declining
	 * to install is the recoverable answer. */
	if (auto* cancel = m_ui->buttonBox->button(QDialogButtonBox::Cancel)) {
		cancel->setDefault(true);
		cancel->setFocus();
	}
}

UntrustedModsDialog::~UntrustedModsDialog()
{
	delete m_ui;
}
