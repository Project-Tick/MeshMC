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

#include "Version.h"

#include <QStringList>
#include <QUrl>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

Version::Version(const QString& str) : m_string(str)
{
	parse();
}

bool Version::operator<(const Version& other) const
{
	const int size = qMax(m_sections.size(), other.m_sections.size());
	for (int i = 0; i < size; ++i) {
		const Section sec1 =
			(i >= m_sections.size()) ? Section("0") : m_sections.at(i);
		const Section sec2 = (i >= other.m_sections.size())
								 ? Section("0")
								 : other.m_sections.at(i);
		if (sec1 != sec2) {
			return sec1 < sec2;
		}
	}

	return false;
}
bool Version::operator<=(const Version& other) const
{
	return *this < other || *this == other;
}
bool Version::operator>(const Version& other) const
{
	const int size = qMax(m_sections.size(), other.m_sections.size());
	for (int i = 0; i < size; ++i) {
		const Section sec1 =
			(i >= m_sections.size()) ? Section("0") : m_sections.at(i);
		const Section sec2 = (i >= other.m_sections.size())
								 ? Section("0")
								 : other.m_sections.at(i);
		if (sec1 != sec2) {
			return sec1 > sec2;
		}
	}

	return false;
}
bool Version::operator>=(const Version& other) const
{
	return *this > other || *this == other;
}
bool Version::operator==(const Version& other) const
{
	const int size = qMax(m_sections.size(), other.m_sections.size());
	for (int i = 0; i < size; ++i) {
		const Section sec1 =
			(i >= m_sections.size()) ? Section("0") : m_sections.at(i);
		const Section sec2 = (i >= other.m_sections.size())
								 ? Section("0")
								 : other.m_sections.at(i);
		if (sec1 != sec2) {
			return false;
		}
	}

	return true;
}
bool Version::operator!=(const Version& other) const
{
	return !operator==(other);
}

void Version::parse()
{
	m_sections.clear();

	// FIXME: this is bad. versions can contain a lot more separators...
	QStringList parts = m_string.split('.');

	for (const auto& part : parts) {
		m_sections.append(Section(part));
	}
}
