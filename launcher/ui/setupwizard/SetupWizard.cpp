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
 */

#include "SetupWizard.h"

#include "LanguageWizardPage.h"
#include "JavaWizardPage.h"
#include "AnalyticsWizardPage.h"

#include "translations/TranslationsModel.h"
#include <Application.h>
#include <FileSystem.h>
#include <ganalytics.h>

#include <QAbstractButton>
#include <BuildConfig.h>

SetupWizard::SetupWizard(QWidget* parent) : QWizard(parent)
{
	setObjectName(QStringLiteral("SetupWizard"));
	resize(615, 659);
	// make it ugly everywhere to avoid variability in theming
	setWizardStyle(QWizard::ClassicStyle);
	setOptions(QWizard::NoCancelButton | QWizard::IndependentPages |
			   QWizard::HaveCustomButton1);

	retranslate();

	connect(this, &QWizard::currentIdChanged, this, &SetupWizard::pageChanged);
}

void SetupWizard::retranslate()
{
	setButtonText(QWizard::NextButton, tr("&Next >"));
	setButtonText(QWizard::BackButton, tr("< &Back"));
	setButtonText(QWizard::FinishButton, tr("&Finish"));
	setButtonText(QWizard::CustomButton1, tr("&Refresh"));
	setWindowTitle(tr("%1 Quick Setup").arg(BuildConfig.MESHMC_NAME));
}

BaseWizardPage* SetupWizard::getBasePage(int id)
{
	if (id == -1)
		return nullptr;
	auto pagePtr = page(id);
	if (!pagePtr)
		return nullptr;
	return dynamic_cast<BaseWizardPage*>(pagePtr);
}

BaseWizardPage* SetupWizard::getCurrentBasePage()
{
	return getBasePage(currentId());
}

void SetupWizard::pageChanged(int id)
{
	auto basePagePtr = getBasePage(id);
	if (!basePagePtr) {
		return;
	}
	if (basePagePtr->wantsRefreshButton()) {
		setButtonLayout({QWizard::CustomButton1, QWizard::Stretch,
						 QWizard::BackButton, QWizard::NextButton,
						 QWizard::FinishButton});
		auto customButton = button(QWizard::CustomButton1);
		connect(customButton, &QAbstractButton::pressed, [&]() {
			auto basePagePtr = getCurrentBasePage();
			if (basePagePtr) {
				basePagePtr->refresh();
			}
		});
	} else {
		setButtonLayout({QWizard::Stretch, QWizard::BackButton,
						 QWizard::NextButton, QWizard::FinishButton});
	}
}

void SetupWizard::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange) {
		retranslate();
	}
	QWizard::changeEvent(event);
}

SetupWizard::~SetupWizard() {}
