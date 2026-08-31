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
