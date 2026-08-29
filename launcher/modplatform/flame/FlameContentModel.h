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

#pragma once

#include "modplatform/ContentProviderModel.h"

/* CurseForge search results. Everything except the shape of the JSON
 * lives in the base class. */
class FlameContentModel final : public ContentProviderModel
{
	Q_OBJECT

  public:
	explicit FlameContentModel(ModPlatform::ContentType contentType,
							   ModPlatform::SearchFilters filters,
							   QObject* parent = nullptr);

  protected:
	QList<ModPlatform::IndexedProject>
	parseSearchResponse(const QByteArray& bytes, int& totalHits) const override;

	ModPlatform::IndexedProject
	parseProjectResponse(const QByteArray& bytes) const override;

	QList<ModPlatform::ContentVersion>
	parseVersionsResponse(const QByteArray& bytes,
						  const ModPlatform::IndexedProject& project)
		const override;

	QString parseBodyResponse(const QByteArray& bytes) const override;

	QList<ModPlatform::Category>
	parseCategoriesResponse(const QByteArray& bytes) const override;

	QString iconCacheName() const override;
};
