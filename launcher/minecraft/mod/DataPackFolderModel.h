/* Copyright 2026 Project Tick
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */
#pragma once

#include "ModFolderModel.h"

/* Backs a `datapacks` folder.
 *
 * Two things use this: the instance-wide global data pack folder (picked
 * up by loaders such as Paxi or OpenLoader) and, in the future, the
 * per-world folder under saves/<world>/datapacks. Both have identical
 * semantics, so the model only needs the folder path. */
class DataPackFolderModel : public ModFolderModel
{
	Q_OBJECT

  public:
	explicit DataPackFolderModel(const QString& dir);

	QVariant headerData(int section, Qt::Orientation orientation,
						int role) const override;

  protected:
	bool acceptsFile(const QFileInfo& file, Mod::ModType type) const override;
};
