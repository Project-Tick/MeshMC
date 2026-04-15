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

#include "LanguageWizardPage.h"
#include <Application.h>
#include <translations/TranslationsModel.h>

#include "ui/widgets/LanguageSelectionWidget.h"
#include <QVBoxLayout>
#include <BuildConfig.h>

LanguageWizardPage::LanguageWizardPage(QWidget* parent) : BaseWizardPage(parent)
{
	setObjectName(QStringLiteral("languagePage"));
	auto layout = new QVBoxLayout(this);
	mainWidget = new LanguageSelectionWidget(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(mainWidget);

	retranslate();
}

LanguageWizardPage::~LanguageWizardPage() {}

bool LanguageWizardPage::wantsRefreshButton()
{
	return true;
}

void LanguageWizardPage::refresh()
{
	auto translations = APPLICATION->translations();
	translations->downloadIndex();
}

bool LanguageWizardPage::validatePage()
{
	auto settings = APPLICATION->settings();
	QString key = mainWidget->getSelectedLanguageKey();
	settings->set("Language", key);
	return true;
}

void LanguageWizardPage::retranslate()
{
	setTitle(tr("Language"));
	setSubTitle(
		tr("Select the language to use in %1").arg(BuildConfig.MESHMC_NAME));
	mainWidget->retranslate();
}
