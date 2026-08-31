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

#include <QVBoxLayout>
#include <QWidget>
#include <memory>

#include "ModFolderPage.h"

class ModFolderModel;

/* Data pack manager for one `datapacks` folder.
 *
 * The folder is passed in rather than derived from the instance, because
 * data packs live in two very different places: vanilla reads them from
 * saves/<world>/datapacks (one folder per world - see WorldListPage) and
 * loaders like Paxi read them from a single instance-wide folder. Both
 * behave identically once you have the model, so one page serves both. */
class DataPackPage : public ModFolderPage
{
	Q_OBJECT
  public:
	explicit DataPackPage(MinecraftInstance* instance,
						  std::shared_ptr<ModFolderModel> model,
						  QWidget* parent = nullptr);
	virtual ~DataPackPage() {}

	virtual bool shouldDisplay() const override
	{
		return true;
	}
};

/* Instance-level page for the global data pack folder.
 *
 * Wraps DataPackPage because the folder it points at can be switched off
 * entirely (GlobalDataPacksEnabled) or moved (GlobalDataPacksPath) from
 * the instance settings while this page already exists. The wrapper
 * throws the inner page away and rebuilds it whenever that happens, so
 * the page can never be left showing a folder that is no longer the
 * configured one. */
class GlobalDataPackPage : public QWidget, public BasePage
{
	Q_OBJECT
  public:
	explicit GlobalDataPackPage(MinecraftInstance* instance,
								QWidget* parent = nullptr);

	virtual QString id() const override
	{
		return "datapacks";
	}
	virtual QString displayName() const override;
	virtual QIcon icon() const override;
	virtual QString helpPage() const override;
	virtual bool shouldDisplay() const override;

	virtual bool apply() override;
	virtual void openedImpl() override;
	virtual void closedImpl() override;
	virtual void setParentContainer(BasePageContainer* container) override;

  private:
	void updateContent();

	MinecraftInstance* m_instance = nullptr;
	DataPackPage* m_underlyingPage = nullptr;
};
