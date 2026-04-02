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

#pragma once

#include "BaseWizardPage.h"

class QVBoxLayout;
class QTextBrowser;
class QCheckBox;

class AnalyticsWizardPage : public BaseWizardPage
{
	Q_OBJECT
  public:
	explicit AnalyticsWizardPage(QWidget* parent = Q_NULLPTR);
	virtual ~AnalyticsWizardPage();

	bool validatePage() override;

  protected:
	void retranslate() override;

  private:
	QVBoxLayout* verticalLayout_3 = nullptr;
	QTextBrowser* textBrowser = nullptr;
	QCheckBox* checkBox = nullptr;
};