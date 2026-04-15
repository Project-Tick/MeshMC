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
 * Copyright 2013-2021 MultiMC Contributors
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

#include "SkinUtils.h"
#include "net/HttpMetaCache.h"
#include "Application.h"

#include <QFile>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace SkinUtils
{
	/*
	 * Given a username, return a pixmap of the cached skin (if it exists),
	 * QPixmap() otherwise
	 */
	QPixmap getFaceFromCache(QString username, int height, int width)
	{
		QFile fskin(APPLICATION->metacache()
						->resolveEntry("skins", username + ".png")
						->getFullPath());

		if (fskin.exists()) {
			QPixmap skinTexture(fskin.fileName());
			if (!skinTexture.isNull()) {
				QPixmap skin = QPixmap(8, 8);
				QPainter painter(&skin);
				painter.drawPixmap(0, 0, skinTexture.copy(8, 8, 8, 8));
				painter.drawPixmap(0, 0, skinTexture.copy(40, 8, 8, 8));
				return skin.scaled(height, width, Qt::KeepAspectRatio);
			}
		}

		return QPixmap();
	}
} // namespace SkinUtils
