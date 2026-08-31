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
