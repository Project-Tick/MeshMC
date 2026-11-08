/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2013-2021 MultiMC Contributors
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

#include "ProxyPage.h"
#include "ui_ProxyPage.h"

#include <QTabBar>

#include "settings/SettingsObject.h"
#include "ui/widgets/ChoiceBinding.h"
#include "Application.h"

ProxyPage::ProxyPage(QWidget* parent) : QWidget(parent), ui(new Ui::ProxyPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	// The proxy type is a choice of radio buttons, a text in the settings.
	// The settings dialog binds it, like the widgets named kcfg_<ID>.
	m_proxyType = new ChoiceBinding("ProxyType", this);
	m_proxyType->addChoice(ui->proxyDefaultBtn, "Default");
	m_proxyType->addChoice(ui->proxyNoneBtn, "None");
	m_proxyType->addChoice(ui->proxySOCKS5Btn, "SOCKS5");
	m_proxyType->addChoice(ui->proxyHTTPBtn, "HTTP");
	// Also when the dialog loads the setting, or resets it to its default
	connect(m_proxyType, &ChoiceBinding::choiceChanged, this,
			&ProxyPage::updateCheckboxStuff);
	updateCheckboxStuff();
}

ProxyPage::~ProxyPage()
{
	delete ui;
}

bool ProxyPage::apply()
{
	applySettings();
	return true;
}

void ProxyPage::updateCheckboxStuff()
{
	ui->proxyAddrBox->setEnabled(!ui->proxyNoneBtn->isChecked() &&
								 !ui->proxyDefaultBtn->isChecked());
	ui->proxyAuthBox->setEnabled(!ui->proxyNoneBtn->isChecked() &&
								 !ui->proxyDefaultBtn->isChecked());
}

void ProxyPage::applySettings()
{
	// The type, the address, the port and the credentials are bound to their
	// settings by the settings dialog, which has saved them when this is
	// called; what is left is to put them to use.
	QString proxyType = m_proxyType->choice();
	if (proxyType.isEmpty()) {
		proxyType = "None";
	}

	APPLICATION->updateProxySettings(
		proxyType, ui->kcfg_ProxyAddr->text(), ui->kcfg_ProxyPort->value(),
		ui->kcfg_ProxyUser->text(), ui->kcfg_ProxyPass->text());
}

