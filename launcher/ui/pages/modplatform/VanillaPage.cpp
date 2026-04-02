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

#include "VanillaPage.h"
#include "ui_VanillaPage.h"

#include <QTabBar>

#include "Application.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "Filter.h"
#include "InstanceCreationTask.h"

VanillaPage::VanillaPage(NewInstanceDialog* dialog, QWidget* parent)
	: QWidget(parent), dialog(dialog), ui(new Ui::VanillaPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	connect(ui->versionList, &VersionSelectWidget::selectedVersionChanged, this,
			&VanillaPage::setSelectedVersion);
	filterChanged();
	connect(ui->alphaFilter, &QCheckBox::stateChanged, this,
			&VanillaPage::filterChanged);
	connect(ui->betaFilter, &QCheckBox::stateChanged, this,
			&VanillaPage::filterChanged);
	connect(ui->snapshotFilter, &QCheckBox::stateChanged, this,
			&VanillaPage::filterChanged);
	connect(ui->oldSnapshotFilter, &QCheckBox::stateChanged, this,
			&VanillaPage::filterChanged);
	connect(ui->releaseFilter, &QCheckBox::stateChanged, this,
			&VanillaPage::filterChanged);
	connect(ui->experimentsFilter, &QCheckBox::stateChanged, this,
			&VanillaPage::filterChanged);
	connect(ui->refreshBtn, &QPushButton::clicked, this, &VanillaPage::refresh);
}

void VanillaPage::openedImpl()
{
	if (!initialized) {
		auto vlist = APPLICATION->metadataIndex()->get("net.minecraft");
		ui->versionList->initialize(vlist.get());
		initialized = true;
	} else {
		suggestCurrent();
	}
}

void VanillaPage::refresh()
{
	ui->versionList->loadList();
}

void VanillaPage::filterChanged()
{
	QStringList out;
	if (ui->alphaFilter->isChecked())
		out << "(old_alpha)";
	if (ui->betaFilter->isChecked())
		out << "(old_beta)";
	if (ui->snapshotFilter->isChecked())
		out << "(snapshot)";
	if (ui->oldSnapshotFilter->isChecked())
		out << "(old_snapshot)";
	if (ui->releaseFilter->isChecked())
		out << "(release)";
	if (ui->experimentsFilter->isChecked())
		out << "(experiment)";
	auto regexp = out.join('|');
	ui->versionList->setFilter(BaseVersionList::TypeRole,
							   new RegexpFilter(regexp, false));
}

VanillaPage::~VanillaPage()
{
	delete ui;
}

bool VanillaPage::shouldDisplay() const
{
	return true;
}

BaseVersionPtr VanillaPage::selectedVersion() const
{
	return m_selectedVersion;
}

void VanillaPage::suggestCurrent()
{
	if (!isOpened) {
		return;
	}

	if (!m_selectedVersion) {
		dialog->setSuggestedPack();
		return;
	}

	dialog->setSuggestedPack(m_selectedVersion->descriptor(),
							 new InstanceCreationTask(m_selectedVersion));
	dialog->setSuggestedIcon("default");
}

void VanillaPage::setSelectedVersion(BaseVersionPtr version)
{
	m_selectedVersion = version;
	suggestCurrent();
}
