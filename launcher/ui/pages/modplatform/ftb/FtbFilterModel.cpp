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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "FtbFilterModel.h"

#include <QDebug>

#include "modplatform/modpacksch/FTBPackManifest.h"
#include <MMCStrings.h>

namespace Ftb
{

	FilterModel::FilterModel(QObject* parent) : QSortFilterProxyModel(parent)
	{
		currentSorting = Sorting::ByPlays;
		sortings.insert(tr("Sort by plays"), Sorting::ByPlays);
		sortings.insert(tr("Sort by installs"), Sorting::ByInstalls);
		sortings.insert(tr("Sort by name"), Sorting::ByName);
	}

	const QMap<QString, FilterModel::Sorting>
	FilterModel::getAvailableSortings()
	{
		return sortings;
	}

	QString FilterModel::translateCurrentSorting()
	{
		return sortings.key(currentSorting);
	}

	void FilterModel::setSorting(Sorting sorting)
	{
		currentSorting = sorting;
		invalidate();
	}

	FilterModel::Sorting FilterModel::getCurrentSorting()
	{
		return currentSorting;
	}

	void FilterModel::setSearchTerm(const QString& term)
	{
		searchTerm = term.trimmed();
		invalidate();
	}

	bool FilterModel::filterAcceptsRow(int sourceRow,
									   const QModelIndex& sourceParent) const
	{
		if (searchTerm.isEmpty()) {
			return true;
		}

		auto index = sourceModel()->index(sourceRow, 0, sourceParent);
		auto pack = sourceModel()
						->data(index, Qt::UserRole)
						.value<ModpacksCH::Modpack>();
		return pack.name.contains(searchTerm, Qt::CaseInsensitive);
	}

	bool FilterModel::lessThan(const QModelIndex& left,
							   const QModelIndex& right) const
	{
		ModpacksCH::Modpack leftPack = sourceModel()
										   ->data(left, Qt::UserRole)
										   .value<ModpacksCH::Modpack>();
		ModpacksCH::Modpack rightPack = sourceModel()
											->data(right, Qt::UserRole)
											.value<ModpacksCH::Modpack>();

		if (currentSorting == ByPlays) {
			return leftPack.plays < rightPack.plays;
		} else if (currentSorting == ByInstalls) {
			return leftPack.installs < rightPack.installs;
		} else if (currentSorting == ByName) {
			return Strings::naturalCompare(leftPack.name, rightPack.name,
										   Qt::CaseSensitive) >= 0;
		}

		// Invalid sorting set, somehow...
		qWarning() << "Invalid sorting set!";
		return true;
	}

} // namespace Ftb
