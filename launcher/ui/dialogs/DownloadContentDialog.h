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
 *
 */

#pragma once

#include <QDialog>
#include <QListView>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>
#include <QTimer>

#include "minecraft/MinecraftInstance.h"
#include "modplatform/ContentType.h"
#include "modplatform/ModDownloadTypes.h"

namespace Flame
{
	class ModListModel;
}
namespace Modrinth
{
	class ModListModel;
}

class DownloadContentDialog : public QDialog
{
	Q_OBJECT

  public:
	explicit DownloadContentDialog(MinecraftInstance* instance,
								   ModPlatform::ContentType contentType,
								   QWidget* parent = nullptr);
	virtual ~DownloadContentDialog();

	QList<ModPlatform::SelectedMod> selectedMods() const
	{
		return m_selectedMods;
	}
	QString mcVersion() const
	{
		return m_mcVersion;
	}
	QString loaderType() const
	{
		return m_loaderType;
	}

  private slots:
	void onPlatformChanged(int index);
	void onSearchTextChanged();
	void onSortChanged(int index);
	void onModSelectionChanged(const QModelIndex& current,
							   const QModelIndex& previous);
	void onModDoubleClicked(const QModelIndex& index);
	void onContinueClicked();
	void onCancelClicked();
	void removeSelectedMod(int index);

  private:
	void setupUi();
	void triggerSearch();
	void updateSelectedList();
	void loadVersionsForMod(const QModelIndex& index);

  private:
	MinecraftInstance* m_instance;
	ModPlatform::ContentType m_contentType;
	QString m_mcVersion;
	QString m_loaderType;

	// UI elements
	QListWidget* m_platformList = nullptr;
	QStackedWidget* m_contentStack = nullptr;
	QLineEdit* m_searchEdit = nullptr;
	QComboBox* m_sortBox = nullptr;
	QListView* m_curseForgeView = nullptr;
	QListView* m_modrinthView = nullptr;
	QComboBox* m_versionBox = nullptr;
	QLabel* m_descriptionLabel = nullptr;
	QListWidget* m_selectedListWidget = nullptr;
	QPushButton* m_continueButton = nullptr;
	QPushButton* m_cancelButton = nullptr;
	QPushButton* m_addButton = nullptr;

	// Models
	Flame::ModListModel* m_flameModel = nullptr;
	Modrinth::ModListModel* m_modrinthModel = nullptr;

	// State
	QList<ModPlatform::SelectedMod> m_selectedMods;
	int m_currentPlatform = 0; // 0=CurseForge, 1=Modrinth
	QTimer* m_searchTimer = nullptr;

	// Currently selected mod info for version loading
	struct CurrentModInfo {
		QString platform;
		QString projectId;
		QString name;
		int addonId = 0; // for CurseForge
	} m_currentMod;
};
