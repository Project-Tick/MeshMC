/*
 * SPDX-FileCopyrightText: 2013, 2015 ljfa-ag <ljfa-ag@web.de>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

/*
 * libnbt++ - A library for the Minecraft Named Binary Tag format.
 * Copyright (C) 2013, 2015  ljfa-ag
 *
 * This file is part of libnbt++.
 *
 * libnbt++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libnbt++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libnbt++.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <cxxtest/TestSuite.h>
#include <cstdint>
#include "value.h"

using namespace nbt;

class value_assignment_test : public CxxTest::TestSuite
{
  public:
	void test_numeric_assignments()
	{
		value v;

		v = int8_t(-5);
		TS_ASSERT_EQUALS(int32_t(v), int32_t(-5));
		TS_ASSERT_EQUALS(double(v), -5.);

		v = value();
		v = int16_t(12345);
		TS_ASSERT_EQUALS(int32_t(v), int32_t(12345));
		TS_ASSERT_EQUALS(double(v), 12345.);

		v = value();
		v = int32_t(100000);
		TS_ASSERT_EQUALS(int64_t(v), int64_t(100000));
		TS_ASSERT_EQUALS(double(v), 100000.);

		v = value();
		v = float(3.14f);
		TS_ASSERT_DELTA(double(v), 3.14, 1e-6);

		v = value();
		v = double(2.718281828);
		TS_ASSERT_EQUALS(double(v), 2.718281828);
	}
};
