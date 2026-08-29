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

#include "DataPackPage.h"
#include "ui_ModFolderPage.h"

#include "settings/Setting.h"
#include "settings/SettingsObject.h"

DataPackPage::DataPackPage(MinecraftInstance* instance,
						   std::shared_ptr<ModFolderModel> model,
						   QWidget* parent)
	: ModFolderPage(instance, std::move(model), "datapacks", "resourcepacks",
					tr("Data packs"), "Data-packs", parent)
{
	ui->actionView_configs->setVisible(false);
	setContentType(ModPlatform::ContentType::DataPack);
}

GlobalDataPackPage::GlobalDataPackPage(MinecraftInstance* instance,
									   QWidget* parent)
	: QWidget(parent), m_instance(instance)
{
	auto* boxLayout = new QVBoxLayout(this);
	boxLayout->setContentsMargins(0, 0, 0, 0);
	setLayout(boxLayout);

	// Toggling the folder on or off changes whether this page exists at
	// all, so the container has to re-evaluate its page list too.
	connect(m_instance->settings()->getSetting("GlobalDataPacksEnabled").get(),
			&Setting::SettingChanged, this, [this] {
				updateContent();
				if (m_container != nullptr) {
					m_container->refreshContainer();
				}
			});

	// Moving the folder only invalidates the inner page.
	connect(m_instance->settings()->getSetting("GlobalDataPacksPath").get(),
			&Setting::SettingChanged, this,
			[this] { updateContent(); });
}

QString GlobalDataPackPage::displayName() const
{
	if (m_underlyingPage == nullptr) {
		return tr("Data packs");
	}
	return m_underlyingPage->displayName();
}

QIcon GlobalDataPackPage::icon() const
{
	if (m_underlyingPage == nullptr) {
		return APPLICATION->getThemedIcon("resourcepacks");
	}
	return m_underlyingPage->icon();
}

QString GlobalDataPackPage::helpPage() const
{
	if (m_underlyingPage == nullptr) {
		return QStringLiteral("Data-packs");
	}
	return m_underlyingPage->helpPage();
}

bool GlobalDataPackPage::shouldDisplay() const
{
	return m_instance->settings()->get("GlobalDataPacksEnabled").toBool();
}

bool GlobalDataPackPage::apply()
{
	return m_underlyingPage == nullptr || m_underlyingPage->apply();
}

void GlobalDataPackPage::openedImpl()
{
	if (m_underlyingPage != nullptr) {
		m_underlyingPage->opened();
	}
}

void GlobalDataPackPage::closedImpl()
{
	if (m_underlyingPage != nullptr) {
		m_underlyingPage->closed();
	}
}

void GlobalDataPackPage::setParentContainer(BasePageContainer* container)
{
	BasePage::setParentContainer(container);
	updateContent();
}

void GlobalDataPackPage::updateContent()
{
	if (m_underlyingPage != nullptr) {
		/* isOpened tracks whether the container currently has us on
		 * screen; the inner page must see a matching closed() before it
		 * goes away so it can stop watching its folder. */
		if (isOpened) {
			m_underlyingPage->closed();
		}
		m_underlyingPage->apply();
		layout()->removeWidget(m_underlyingPage);
		delete m_underlyingPage;
		m_underlyingPage = nullptr;
	}

	if (!shouldDisplay()) {
		return;
	}

	auto model = m_instance->dataPackList();
	if (!model) {
		// Setting says enabled but the model could not be built; nothing
		// sensible to show.
		return;
	}

	m_underlyingPage = new DataPackPage(m_instance, std::move(model));
	m_underlyingPage->setParentContainer(m_container);
	layout()->addWidget(m_underlyingPage);

	if (isOpened) {
		m_underlyingPage->opened();
	}
}
