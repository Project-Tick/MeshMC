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

#include "GlobalSettingsDialog.h"
#include "settings/GlobalSettings.h"

#include <QDebug>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QUrl>

#include <KCoreConfigSkeleton>
#include <KPageWidgetModel>

#include "Application.h"
#include "DesktopServices.h"
#include "settings/SettingsObject.h"

GlobalSettingsDialog::GlobalSettingsDialog(BasePageProvider* pageProvider,
										   KCoreConfigSkeleton* skeleton,
										   const QString& defaultId,
										   QWidget* parent)
	: KConfigDialog(parent, QStringLiteral("meshmc-global-settings"), skeleton)
{
	setWindowModality(Qt::WindowModal);
	setWindowTitle(pageProvider->dialogTitle());
	setFaceType(KPageDialog::List);

	// The items of the skeleton hold what the settings were when it was built,
	// or when the dialog was last open. They have to show what they are now.
	skeleton->load();

	connect(this, &KPageDialog::currentPageChanged, this,
			&GlobalSettingsDialog::pageChanged);

	m_pages = pageProvider->getPages();
	for (BasePage* page : m_pages) {
		QWidget* widget = dynamic_cast<QWidget*>(page);
		if (!widget) {
			qWarning() << "Settings page" << page->id()
					   << "is not a widget; it is left out.";
			continue;
		}
		page->setParentContainer(this);
		// Widgets named kcfg_<ID> are bound to the setting with that ID.
		KPageWidgetItem* item =
			addPage(widget, skeleton, page->displayName());
		item->setIcon(page->icon());
		item->setHeader(page->displayName());
		item->setEnabled(page->shouldDisplay());
		m_pageOfItem.insert(item, page);
		m_itemOfId.insert(page->id(), item);
	}
	selectPage(defaultId);

	if (!buttonBox()->button(QDialogButtonBox::Help)) {
		auto* help = buttonBox()->addButton(QDialogButtonBox::Help);
		connect(help, &QPushButton::clicked, this,
				&GlobalSettingsDialog::showHelp);
	}

	restoreGeometry(QByteArray::fromBase64(
		APPLICATION->globalSettings()->pagedGeometry().toUtf8()));
}

GlobalSettingsDialog::~GlobalSettingsDialog() {}

BasePage* GlobalSettingsDialog::pageOf(KPageWidgetItem* item) const
{
	return m_pageOfItem.value(item, nullptr);
}

bool GlobalSettingsDialog::selectPage(QString pageId)
{
	KPageWidgetItem* item = m_itemOfId.value(pageId, nullptr);
	if (!item || !item->isEnabled()) {
		// like PageContainer: the first page that can be shown
		item = nullptr;
		for (BasePage* page : m_pages) {
			KPageWidgetItem* candidate = m_itemOfId.value(page->id(), nullptr);
			if (candidate && candidate->isEnabled()) {
				item = candidate;
				break;
			}
		}
	}
	if (!item) {
		return false;
	}
	setCurrentPage(item);
	return true;
}

void GlobalSettingsDialog::refreshContainer()
{
	for (BasePage* page : m_pages) {
		if (KPageWidgetItem* item = m_itemOfId.value(page->id(), nullptr)) {
			item->setEnabled(page->shouldDisplay());
		}
	}
	if (m_currentPage && !m_currentPage->shouldDisplay()) {
		selectPage(QString());
	}
}

bool GlobalSettingsDialog::requestClose()
{
	return close();
}

void GlobalSettingsDialog::pageChanged(KPageWidgetItem* current,
									   KPageWidgetItem* before)
{
	if (BasePage* page = pageOf(before)) {
		page->closed();
	}
	m_currentPage = pageOf(current);
	if (m_currentPage) {
		m_currentPage->opened();
	}
}

bool GlobalSettingsDialog::hasChanged()
{
	return true;
}

void GlobalSettingsDialog::updateSettings()
{
	if (m_savedByDone) {
		// OK: done() has just done this, see there.
		m_savedByDone = false;
		return;
	}
	// the widgets named kcfg_<ID>
	KConfigDialog::updateSettings();
	// what the pages do themselves
	for (BasePage* page : m_pages) {
		if (!page->apply()) {
			qWarning() << "Settings page" << page->id()
					   << "could not apply its settings.";
		}
	}
}

void GlobalSettingsDialog::showHelp()
{
	if (!m_currentPage) {
		return;
	}
	const QString pageId = m_currentPage->helpPage();
	if (pageId.isEmpty()) {
		return;
	}
	DesktopServices::openUrl(
		QUrl("https://github.com/Project-Tick/MeshMC/wiki/" + pageId));
}

void GlobalSettingsDialog::done(int result)
{
	// KConfigDialog saves on OK only after done() has returned, but the pages
	// have to be applied before they are told they were closed: that is the
	// order BasePage promised. So OK does it here, and the call that follows
	// from the button is skipped (the dialog is gone by then, nothing else can
	// be affected by the flag).
	if (result == QDialog::Accepted) {
		updateSettings();
		m_savedByDone = true;
	}
	KConfigDialog::done(result);

	if (m_currentPage) {
		m_currentPage->closed();
		m_currentPage = nullptr;
	}
	APPLICATION->globalSettings()->setPagedGeometry(
		QString::fromLatin1(saveGeometry().toBase64()));
	APPLICATION->globalSettings()->save();
}
