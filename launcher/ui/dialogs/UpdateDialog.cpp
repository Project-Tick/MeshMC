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

#include "UpdateDialog.h"
#include "ui_UpdateDialog.h"
#include "Application.h"
#include "BuildConfig.h"

UpdateDialog::UpdateDialog(bool hasUpdate, const UpdateAvailableStatus& status,
						   QWidget* parent)
	: QDialog(parent), ui(new Ui::UpdateDialog)
{
	ui->setupUi(this);

	if (hasUpdate) {
		ui->label->setText(
			tr("<b>%1 %2</b> is available!")
				.arg(BuildConfig.MESHMC_DISPLAYNAME, status.version));

		if (!status.releaseNotes.isEmpty()) {
			ui->changelogBrowser->setHtml(status.releaseNotes);
		} else {
			ui->changelogBrowser->setHtml(
				tr("<center><p>No release notes available.</p></center>"));
		}
	} else {
		ui->label->setText(tr("You are running the latest version of %1.")
							   .arg(BuildConfig.MESHMC_DISPLAYNAME));
		ui->changelogBrowser->setHtml(
			tr("<center><p>No updates found.</p></center>"));
		ui->btnUpdateNow->setHidden(true);
		ui->btnUpdateLater->setText(tr("Close"));
	}

	restoreGeometry(QByteArray::fromBase64(
		APPLICATION->settings()->get("UpdateDialogGeometry").toByteArray()));
}

UpdateDialog::~UpdateDialog()
{
	delete ui;
}

void UpdateDialog::on_btnUpdateLater_clicked()
{
	reject();
}

void UpdateDialog::on_btnUpdateNow_clicked()
{
	done(UPDATE_NOW);
}

void UpdateDialog::closeEvent(QCloseEvent* evt)
{
	APPLICATION->settings()->set("UpdateDialogGeometry",
								 saveGeometry().toBase64());
	QDialog::closeEvent(evt);
}
