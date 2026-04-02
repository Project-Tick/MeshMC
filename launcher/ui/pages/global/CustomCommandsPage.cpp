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

#include "CustomCommandsPage.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QTabBar>

CustomCommandsPage::CustomCommandsPage(QWidget* parent) : QWidget(parent)
{

	auto verticalLayout = new QVBoxLayout(this);
	verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
	verticalLayout->setContentsMargins(0, 0, 0, 0);

	auto tabWidget = new QTabWidget(this);
	tabWidget->setObjectName(QStringLiteral("tabWidget"));
	commands = new CustomCommands(this);
	commands->setContentsMargins(6, 6, 6, 6);
	tabWidget->addTab(commands, "Foo");
	tabWidget->tabBar()->hide();
	verticalLayout->addWidget(tabWidget);
	loadSettings();
}

CustomCommandsPage::~CustomCommandsPage() {}

bool CustomCommandsPage::apply()
{
	applySettings();
	return true;
}

void CustomCommandsPage::applySettings()
{
	auto s = APPLICATION->settings();
	s->set("PreLaunchCommand", commands->prelaunchCommand());
	s->set("WrapperCommand", commands->wrapperCommand());
	s->set("PostExitCommand", commands->postexitCommand());
}

void CustomCommandsPage::loadSettings()
{
	auto s = APPLICATION->settings();
	commands->initialize(false, true, s->get("PreLaunchCommand").toString(),
						 s->get("WrapperCommand").toString(),
						 s->get("PostExitCommand").toString());
}
