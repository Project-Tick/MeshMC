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

#include <QDir>
#include <QString>

#include "Mod.h"
#include <QDebug>
#include <FileSystem.h>

namespace
{

	ModDetails invalidDetails;

}

Mod::Mod(const QFileInfo& file)
{
	repath(file);
	m_changedDateTime = file.lastModified();
}

void Mod::repath(const QFileInfo& file)
{
	m_file = file;
	QString name_base = file.fileName();

	m_type = Mod::MOD_UNKNOWN;

	m_mmc_id = name_base;

	if (m_file.isDir()) {
		m_type = MOD_FOLDER;
		m_name = name_base;
	} else if (m_file.isFile()) {
		if (name_base.endsWith(".disabled")) {
			m_enabled = false;
			name_base.chop(9);
		} else {
			m_enabled = true;
		}
		if (name_base.endsWith(".zip") || name_base.endsWith(".jar")) {
			m_type = MOD_ZIPFILE;
			name_base.chop(4);
		} else if (name_base.endsWith(".litemod")) {
			m_type = MOD_LITEMOD;
			name_base.chop(8);
		} else {
			m_type = MOD_SINGLEFILE;
		}
		m_name = name_base;
	}
}

bool Mod::enable(bool value)
{
	if (m_type == Mod::MOD_UNKNOWN || m_type == Mod::MOD_FOLDER)
		return false;

	if (m_enabled == value)
		return false;

	QString path = m_file.absoluteFilePath();
	if (value) {
		QFile foo(path);
		if (!path.endsWith(".disabled"))
			return false;
		path.chop(9);
		if (!foo.rename(path))
			return false;
	} else {
		QFile foo(path);
		path += ".disabled";
		if (!foo.rename(path))
			return false;
	}
	repath(QFileInfo(path));
	m_enabled = value;
	return true;
}

bool Mod::destroy()
{
	m_type = MOD_UNKNOWN;
	return FS::deletePath(m_file.filePath());
}

const ModDetails& Mod::details() const
{
	if (!m_localDetails)
		return invalidDetails;
	return *m_localDetails;
}

QString Mod::version() const
{
	return details().version;
}

QString Mod::name() const
{
	auto& d = details();
	if (!d.name.isEmpty()) {
		return d.name;
	}
	return m_name;
}

QString Mod::homeurl() const
{
	return details().homeurl;
}

QString Mod::description() const
{
	return details().description;
}

QStringList Mod::authors() const
{
	return details().authors;
}
