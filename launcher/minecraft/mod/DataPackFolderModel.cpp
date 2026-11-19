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

#include "DataPackFolderModel.h"

#include <QDebug>

#include "PackLayout.h"

DataPackFolderModel::DataPackFolderModel(const QString& dir)
	: ModFolderModel(dir)
{
}

QVariant DataPackFolderModel::headerData(int section,
										 Qt::Orientation orientation,
										 int role) const
{
	if (role == Qt::ToolTipRole) {
		switch (section) {
			case ActiveColumn:
				return tr("Is the data pack enabled?");
			case NameColumn:
				return tr("The name of the data pack.");
			case VersionColumn:
				return tr("The version of the data pack.");
			case DateColumn:
				return tr("The date and time this data pack was last "
						  "changed (or added).");
			default:
				return QVariant();
		}
	}

	return ModFolderModel::headerData(section, orientation, role);
}

bool DataPackFolderModel::acceptsFile(const QFileInfo& file,
									  Mod::ModType type) const
{
	if (type != Mod::MOD_ZIPFILE && type != Mod::MOD_FOLDER) {
		return false;
	}

	if (!PackLayout::isDataPack(file)) {
		qWarning() << file.filePath()
				   << "has no pack .mcmeta/data pair, refusing to install it "
					  "as a data pack";
		return false;
	}
	return true;
}
