/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
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
