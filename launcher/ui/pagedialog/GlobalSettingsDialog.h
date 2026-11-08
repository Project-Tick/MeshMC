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

#pragma once

#include <KConfigDialog>
#include <QHash>

#include "ui/pages/BasePageContainer.h"
#include "ui/pages/BasePageProvider.h"

class KPageWidgetItem;

/*!
 * \brief The settings dialog of the launcher, a KConfigDialog.
 *
 * The pages come from a BasePageProvider, as they always did, and plugins can
 * add their own. Each page is a page of the dialog. A widget on a page that is
 * named "kcfg_<ID>" is bound to the global setting with that ID by
 * KConfigDialogManager: it shows the value, is marked as modified when it is
 * changed, is saved by Apply and OK, and is reset by Defaults, without the page
 * having to do anything. What a page does itself, in BasePage::apply(), is
 * done on Apply and OK as well.
 *
 * It is the BasePageContainer of its pages, so BasePage::setParentContainer(),
 * opened() and closed() work as they do in a PageContainer.
 */
class GlobalSettingsDialog : public KConfigDialog, public BasePageContainer
{
	Q_OBJECT
  public:
	/*!
	 * \a skeleton the global settings, see KConfigSettingsObject::skeleton().
	 */
	GlobalSettingsDialog(BasePageProvider* pageProvider,
						 KCoreConfigSkeleton* skeleton,
						 const QString& defaultId = QString(),
						 QWidget* parent = nullptr);
	~GlobalSettingsDialog() override;

	bool selectPage(QString pageId) override;
	void refreshContainer() override;
	bool requestClose() override;

  protected slots:
	void updateSettings() override;
	//! Opens the wiki page of the current page.
	void showHelp() override;

  protected:
	// Apply is always on: the pages that do their own work in apply() cannot
	// say whether they have changed.
	bool hasChanged() override;
	void done(int result) override;

  private:
	void pageChanged(KPageWidgetItem* current, KPageWidgetItem* before);
	BasePage* pageOf(KPageWidgetItem* item) const;

	QList<BasePage*> m_pages;
	QHash<KPageWidgetItem*, BasePage*> m_pageOfItem;
	QHash<QString, KPageWidgetItem*> m_itemOfId;
	BasePage* m_currentPage = nullptr;
	bool m_savedByDone = false;
};
