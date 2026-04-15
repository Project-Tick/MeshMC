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

#include <QString>

// NOTE: apparently the GNU C library pollutes the global namespace with
// these... undef them.
#ifdef major
#undef major
#endif
#ifdef minor
#undef minor
#endif

class JavaVersion
{
	friend class JavaVersionTest;

  public:
	JavaVersion() {};
	JavaVersion(const QString& rhs);

	JavaVersion& operator=(const QString& rhs);

	bool operator<(const JavaVersion& rhs) const;
	bool operator==(const JavaVersion& rhs) const;
	bool operator>(const JavaVersion& rhs) const;

	bool requiresPermGen() const;

	QString toString() const;

	int major() const
	{
		return m_major;
	}
	int minor() const
	{
		return m_minor;
	}
	int security() const
	{
		return m_security;
	}

  private:
	QString m_string;
	int m_major = 0;
	int m_minor = 0;
	int m_security = 0;
	bool m_parseable = false;
	QString m_prerelease;
};
