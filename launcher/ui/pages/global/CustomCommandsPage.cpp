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
