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

#include <QSet>
#include <QString>
#include <QFile>

namespace LegacyFTB
{

	class PrivatePackManager
	{
	  public:
		~PrivatePackManager()
		{
			save();
		}
		void load();
		void save() const;
		bool empty() const
		{
			return currentPacks.empty();
		}
		const QSet<QString>& getCurrentPackCodes() const
		{
			return currentPacks;
		}
		void add(const QString& code)
		{
			currentPacks.insert(code);
			dirty = true;
		}
		void remove(const QString& code)
		{
			currentPacks.remove(code);
			dirty = true;
		}

	  private:
		QSet<QString> currentPacks;
		QString m_filename = "private_packs.txt";
		mutable bool dirty = false;
	};

} // namespace LegacyFTB
