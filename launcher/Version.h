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
