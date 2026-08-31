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
#include <SeparatorPrefixTree.h>
#include <QRegularExpression>

class MultiMatcher : public IPathMatcher
{
  public:
	virtual ~MultiMatcher() {};
	MultiMatcher() {}
	MultiMatcher& add(Ptr add)
	{
		m_matchers.append(add);
		return *this;
	}

	virtual bool matches(const QString& string) const override
	{
		for (auto iter : m_matchers) {
			if (iter->matches(string)) {
				return true;
			}
		}
		return false;
	}

	QList<Ptr> m_matchers;
};
