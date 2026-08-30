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
