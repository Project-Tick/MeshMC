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

#include "AtlFilterModel.h"

#include <QDebug>

#include <modplatform/atlauncher/ATLPackIndex.h>
#include <Version.h>
#include <MMCStrings.h>

namespace Atl
{

	FilterModel::FilterModel(QObject* parent) : QSortFilterProxyModel(parent)
	{
		currentSorting = Sorting::ByPopularity;
		sortings.insert(tr("Sort by popularity"), Sorting::ByPopularity);
		sortings.insert(tr("Sort by name"), Sorting::ByName);
		sortings.insert(tr("Sort by game version"), Sorting::ByGameVersion);

		searchTerm = "";
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

	void FilterModel::setSearchTerm(const QString term)
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

		QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
		ATLauncher::IndexedPack pack = sourceModel()
										   ->data(index, Qt::UserRole)
										   .value<ATLauncher::IndexedPack>();
		return pack.name.contains(searchTerm, Qt::CaseInsensitive);
	}

	bool FilterModel::lessThan(const QModelIndex& left,
							   const QModelIndex& right) const
	{
		ATLauncher::IndexedPack leftPack =
			sourceModel()
				->data(left, Qt::UserRole)
				.value<ATLauncher::IndexedPack>();
		ATLauncher::IndexedPack rightPack =
			sourceModel()
				->data(right, Qt::UserRole)
				.value<ATLauncher::IndexedPack>();

		if (currentSorting == ByPopularity) {
			return leftPack.position > rightPack.position;
		} else if (currentSorting == ByGameVersion) {
			Version lv(leftPack.versions.at(0).minecraft);
			Version rv(rightPack.versions.at(0).minecraft);
			return lv < rv;
		} else if (currentSorting == ByName) {
			return Strings::naturalCompare(leftPack.name, rightPack.name,
										   Qt::CaseSensitive) >= 0;
		}

		// Invalid sorting set, somehow...
		qWarning() << "Invalid sorting set!";
		return true;
	}

} // namespace Atl
