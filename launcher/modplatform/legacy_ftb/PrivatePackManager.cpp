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

#include "PrivatePackManager.h"

#include <QDebug>

#include "FileSystem.h"

namespace LegacyFTB
{

	void PrivatePackManager::load()
	{
		try {
			auto parts = QString::fromUtf8(FS::read(m_filename))
							 .split('\n', Qt::SkipEmptyParts);
			currentPacks = QSet<QString>(parts.begin(), parts.end());
			dirty = false;
		} catch (...) {
			currentPacks = {};
			qWarning() << "Failed to read third party FTB pack codes from"
					   << m_filename;
		}
	}

	void PrivatePackManager::save() const
	{
		if (!dirty) {
			return;
		}
		try {
			QStringList list = currentPacks.values();
			FS::write(m_filename, list.join('\n').toUtf8());
			dirty = false;
		} catch (...) {
			qWarning() << "Failed to write third party FTB pack codes to"
					   << m_filename;
		}
	}

} // namespace LegacyFTB
