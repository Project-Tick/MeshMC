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

#include <QWidget>
#include "ui/pages/BasePage.h"
#include <Application.h>

namespace Ui
{
	class AppearancePage;
}

class AppearancePage : public QWidget, public BasePage
{
	Q_OBJECT

  public:
	explicit AppearancePage(QWidget* parent = nullptr);
	~AppearancePage();

	QString displayName() const override
	{
		return tr("Appearance");
	}
	QIcon icon() const override
	{
		return APPLICATION->getThemedIcon("appearance");
	}
	QString id() const override
	{
		return "appearance-settings";
	}
	QString helpPage() const override
	{
		return "Appearance-settings";
	}
	bool apply() override;

  private slots:
	void applyWidgetTheme(int index);
	void applyIconTheme(int index);
	void applyCatTheme(int index);

  private:
	void applySettings();
	void loadSettings();
	void updateIconPreview();
	void updateCatPreview();

	Ui::AppearancePage* ui;
};
