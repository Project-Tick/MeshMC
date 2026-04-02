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

#include "IPathMatcher.h"
#include <QRegularExpression>

class RegexpMatcher : public IPathMatcher
{
  public:
	virtual ~RegexpMatcher() {};
	RegexpMatcher(const QString& regexp)
	{
		m_regexp.setPattern(regexp);
		m_onlyFilenamePart = !regexp.contains('/');
	}

	RegexpMatcher& caseSensitive(bool cs = true)
	{
		if (cs) {
			m_regexp.setPatternOptions(
				QRegularExpression::CaseInsensitiveOption);
		} else {
			m_regexp.setPatternOptions(QRegularExpression::NoPatternOption);
		}
		return *this;
	}

	virtual bool matches(const QString& string) const override
	{
		if (m_onlyFilenamePart) {
			auto slash = string.lastIndexOf('/');
			if (slash != -1) {
				auto part = string.mid(slash + 1);
				return m_regexp.match(part).hasMatch();
			}
		}
		return m_regexp.match(string).hasMatch();
	}
	QRegularExpression m_regexp;
	bool m_onlyFilenamePart = false;
};
