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

#include "IconUtils.h"

#include "FileSystem.h"
#include <QDirIterator>

#include <array>

namespace
{
	std::array<const char*, 6> validIconExtensions = {
		{"svg", "png", "ico", "gif", "jpg", "jpeg"}};
}

namespace IconUtils
{

	QString findBestIconIn(const QString& folder, const QString& iconKey)
	{
		int best_found = validIconExtensions.size();
		QString best_filename;

		QDirIterator it(folder, QDir::NoDotAndDotDot | QDir::Files,
						QDirIterator::NoIteratorFlags);
		while (it.hasNext()) {
			it.next();
			auto fileInfo = it.fileInfo();

			if (fileInfo.completeBaseName() != iconKey)
				continue;

			auto extension = fileInfo.suffix();

			for (int i = 0; i < best_found; i++) {
				if (extension == validIconExtensions[i]) {
					best_found = i;
					qDebug() << i << " : " << fileInfo.fileName();
					best_filename = fileInfo.fileName();
				}
			}
		}
		return FS::PathCombine(folder, best_filename);
	}

	QString getIconFilter()
	{
		QString out;
		QTextStream stream(&out);
		stream << '(';
		for (size_t i = 0; i < validIconExtensions.size() - 1; i++) {
			if (i > 0) {
				stream << " ";
			}
			stream << "*." << validIconExtensions[i];
		}
		stream << " *." << validIconExtensions[validIconExtensions.size() - 1];
		stream << ')';
		return out;
	}

} // namespace IconUtils
