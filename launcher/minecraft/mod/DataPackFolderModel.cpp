/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
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
