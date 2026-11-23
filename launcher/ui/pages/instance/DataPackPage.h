/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
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
