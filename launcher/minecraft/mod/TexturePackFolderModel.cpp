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
 */

#include "TexturePackFolderModel.h"

TexturePackFolderModel::TexturePackFolderModel(const QString& dir)
	: ModFolderModel(dir)
{
}

QVariant TexturePackFolderModel::headerData(int section,
											Qt::Orientation orientation,
											int role) const
{
	if (role == Qt::ToolTipRole) {
		switch (section) {
			case ActiveColumn:
				return tr("Is the texture pack enabled?");
			case NameColumn:
				return tr("The name of the texture pack.");
			case VersionColumn:
				return tr("The version of the texture pack.");
			case DateColumn:
				return tr("The date and time this texture pack was last "
						  "changed (or added).");
			default:
				return QVariant();
		}
	}

	return ModFolderModel::headerData(section, orientation, role);
}
