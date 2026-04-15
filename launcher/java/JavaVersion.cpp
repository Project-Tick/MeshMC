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

#include "JavaVersion.h"
#include <MMCStrings.h>

#include <QRegularExpression>
#include <QString>

JavaVersion& JavaVersion::operator=(const QString& javaVersionString)
{
	m_string = javaVersionString;

	auto getCapturedInteger = [](const QRegularExpressionMatch& match,
								 const QString& what) -> int {
		auto str = match.captured(what);
		if (str.isEmpty()) {
			return 0;
		}
		return str.toInt();
	};

	QRegularExpression pattern;
	if (javaVersionString.startsWith("1.")) {
		pattern = QRegularExpression(
			"1[.](?<major>[0-9]+)([.](?<minor>[0-9]+))?(_(?<security>[0-9]+)?)?"
			"(-(?<prerelease>[a-zA-Z0-9]+))?");
	} else {
		pattern = QRegularExpression(
			"(?<major>[0-9]+)([.](?<minor>[0-9]+))?([.](?<security>[0-9]+))?(-("
			"?<prerelease>[a-zA-Z0-9]+))?");
	}

	auto match = pattern.match(m_string);
	m_parseable = match.hasMatch();
	m_major = getCapturedInteger(match, "major");
	m_minor = getCapturedInteger(match, "minor");
	m_security = getCapturedInteger(match, "security");
	m_prerelease = match.captured("prerelease");
	return *this;
}

JavaVersion::JavaVersion(const QString& rhs)
{
	operator=(rhs);
}

QString JavaVersion::toString() const
{
	return m_string;
}

bool JavaVersion::requiresPermGen() const
{
	if (m_parseable) {
		return m_major < 8;
	}
	return true;
}

bool JavaVersion::operator<(const JavaVersion& rhs) const
{
	if (m_parseable && rhs.m_parseable) {
		auto major = m_major;
		auto rmajor = rhs.m_major;

		// HACK: discourage using java 9
		if (major > 8)
			major = -major;
		if (rmajor > 8)
			rmajor = -rmajor;

		if (major < rmajor)
			return true;
		if (major > rmajor)
			return false;
		if (m_minor < rhs.m_minor)
			return true;
		if (m_minor > rhs.m_minor)
			return false;
		if (m_security < rhs.m_security)
			return true;
		if (m_security > rhs.m_security)
			return false;

		// everything else being equal, consider prerelease status
		bool thisPre = !m_prerelease.isEmpty();
		bool rhsPre = !rhs.m_prerelease.isEmpty();
		if (thisPre && !rhsPre) {
			// this is a prerelease and the other one isn't -> lesser
			return true;
		} else if (!thisPre && rhsPre) {
			// this isn't a prerelease and the other one is -> greater
			return false;
		} else if (thisPre && rhsPre) {
			// both are prereleases - use natural compare...
			return Strings::naturalCompare(m_prerelease, rhs.m_prerelease,
										   Qt::CaseSensitive) < 0;
		}
		// neither is prerelease, so they are the same -> this cannot be less
		// than rhs
		return false;
	} else
		return Strings::naturalCompare(m_string, rhs.m_string,
									   Qt::CaseSensitive) < 0;
}

bool JavaVersion::operator==(const JavaVersion& rhs) const
{
	if (m_parseable && rhs.m_parseable) {
		return m_major == rhs.m_major && m_minor == rhs.m_minor &&
			   m_security == rhs.m_security && m_prerelease == rhs.m_prerelease;
	}
	return m_string == rhs.m_string;
}

bool JavaVersion::operator>(const JavaVersion& rhs) const
{
	return (!operator<(rhs)) && (!operator==(rhs));
}
