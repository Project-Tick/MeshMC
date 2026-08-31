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

#include <modplatform/legacy_ftb/PackHelpers.h>
#include <RWStorage.h>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QThreadPool>
#include <QIcon>
#include <QStyledItemDelegate>

#include <functional>

namespace LegacyFTB
{

	typedef QMap<QString, QIcon> FTBLogoMap;
	typedef std::function<void(QString)> LogoCallback;

	class FilterModel : public QSortFilterProxyModel
	{
		Q_OBJECT
	  public:
		FilterModel(QObject* parent = Q_NULLPTR);
		enum Sorting { ByName, ByGameVersion };
		const QMap<QString, Sorting> getAvailableSortings();
		QString translateCurrentSorting();
		void setSorting(Sorting sorting);
		Sorting getCurrentSorting();

	  protected:
		bool filterAcceptsRow(int sourceRow,
							  const QModelIndex& sourceParent) const override;
		bool lessThan(const QModelIndex& left,
					  const QModelIndex& right) const override;

	  private:
		QMap<QString, Sorting> sortings;
		Sorting currentSorting;
	};

	class ListModel : public QAbstractListModel
	{
		Q_OBJECT
	  private:
		ModpackList modpacks;
		QStringList m_failedLogos;
		QStringList m_loadingLogos;
		FTBLogoMap m_logoMap;
		QMap<QString, LogoCallback> waitingCallbacks;

		void requestLogo(QString file);
		QString translatePackType(PackType type) const;

	  private slots:
		void logoFailed(QString logo);
		void logoLoaded(QString logo, QIcon out);

	  public:
		ListModel(QObject* parent);
		~ListModel();
		int rowCount(const QModelIndex& parent) const override;
		int columnCount(const QModelIndex& parent) const override;
		QVariant data(const QModelIndex& index, int role) const override;
		Qt::ItemFlags flags(const QModelIndex& index) const override;

		void fill(ModpackList modpacks);
		void addPack(Modpack modpack);
		void clear();
		void remove(int row);

		Modpack at(int row);
		void getLogo(const QString& logo, LogoCallback callback);
	};

} // namespace LegacyFTB
