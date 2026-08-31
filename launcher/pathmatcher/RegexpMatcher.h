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
