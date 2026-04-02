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

#include <QString>
#include <QList>

class QUrl;

class Version
{
  public:
	Version(const QString& str);
	Version() {}

	bool operator<(const Version& other) const;
	bool operator<=(const Version& other) const;
	bool operator>(const Version& other) const;
	bool operator>=(const Version& other) const;
	bool operator==(const Version& other) const;
	bool operator!=(const Version& other) const;

	QString toString() const
	{
		return m_string;
	}

  private:
	QString m_string;
	struct Section {
		explicit Section(const QString& fullString)
		{
			m_fullString = fullString;
			int cutoff = m_fullString.size();
			for (int i = 0; i < m_fullString.size(); i++) {
				if (!m_fullString[i].isDigit()) {
					cutoff = i;
					break;
				}
			}
			auto numPart = QStringView{m_fullString}.left(cutoff);
			if (numPart.size()) {
				numValid = true;
				m_numPart = numPart.toInt();
			}
			auto stringPart = QStringView{m_fullString}.mid(cutoff);
			if (stringPart.size()) {
				m_stringPart = stringPart.toString();
			}
		}
		explicit Section() {}
		bool numValid = false;
		int m_numPart = 0;
		QString m_stringPart;
		QString m_fullString;

		inline bool operator!=(const Section& other) const
		{
			if (numValid && other.numValid) {
				return m_numPart != other.m_numPart ||
					   m_stringPart != other.m_stringPart;
			} else {
				return m_fullString != other.m_fullString;
			}
		}
		inline bool operator<(const Section& other) const
		{
			if (numValid && other.numValid) {
				if (m_numPart < other.m_numPart)
					return true;
				if (m_numPart == other.m_numPart &&
					m_stringPart < other.m_stringPart)
					return true;
				return false;
			} else {
				return m_fullString < other.m_fullString;
			}
		}
		inline bool operator>(const Section& other) const
		{
			if (numValid && other.numValid) {
				if (m_numPart > other.m_numPart)
					return true;
				if (m_numPart == other.m_numPart &&
					m_stringPart > other.m_stringPart)
					return true;
				return false;
			} else {
				return m_fullString > other.m_fullString;
			}
		}
	};
	QList<Section> m_sections;

	void parse();
};
