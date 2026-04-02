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

#pragma once

#include "BaseVersion.h"
#include "JavaVersion.h"

struct JavaInstall : public BaseVersion {
	JavaInstall() {}
	JavaInstall(QString id, QString arch, QString path)
		: id(id), arch(arch), path(path)
	{
	}
	virtual QString descriptor()
	{
		return id.toString();
	}

	virtual QString name()
	{
		return id.toString();
	}

	virtual QString typeString() const
	{
		return arch;
	}

	using BaseVersion::operator<;
	using BaseVersion::operator>;

	bool operator<(const JavaInstall& rhs);
	bool operator==(const JavaInstall& rhs);
	bool operator>(const JavaInstall& rhs);

	JavaVersion id;
	QString arch;
	QString path;
	bool recommended = false;
};

typedef std::shared_ptr<JavaInstall> JavaInstallPtr;
