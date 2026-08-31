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

template <typename T> inline void clamp(T& current, T min, T max)
{
	if (current < min) {
		current = min;
	} else if (current > max) {
		current = max;
	}
}

// List of numbers from min to max. Next is exponent times bigger than previous.

class ExponentialSeries
{
  public:
	ExponentialSeries(unsigned min, unsigned max, unsigned exponent = 2)
	{
		m_current = m_min = min;
		m_max = max;
		m_exponent = exponent;
	}
	void reset()
	{
		m_current = m_min;
	}
	unsigned operator()()
	{
		unsigned retval = m_current;
		m_current *= m_exponent;
		clamp(m_current, m_min, m_max);
		return retval;
	}
	unsigned m_current;
	unsigned m_min;
	unsigned m_max;
	unsigned m_exponent;
};
