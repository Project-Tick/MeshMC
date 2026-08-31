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
