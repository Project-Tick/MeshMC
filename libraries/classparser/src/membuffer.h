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
#include <stdint.h>
#include <string>
#include <vector>
#include <exception>
#include "javaendian.h"

namespace util
{
	class membuffer
	{
	  public:
		membuffer(char* buffer, std::size_t size)
		{
			current = start = buffer;
			end = start + size;
		}
		~membuffer()
		{
			// maybe? possibly? left out to avoid confusion. for now.
			// delete start;
		}
		/**
		 * Read some value. That's all ;)
		 */
		template <class T> void read(T& val)
		{
			val = *(T*)current;
			current += sizeof(T);
		}
		/**
		 * Read a big-endian number
		 * valid for 2-byte, 4-byte and 8-byte variables
		 */
		template <class T> void read_be(T& val)
		{
			val = util::bigswap(*(T*)current);
			current += sizeof(T);
		}
		/**
		 * Read a string in the format:
		 * 2B length (big endian, unsigned)
		 * length bytes data
		 */
		void read_jstr(std::string& str)
		{
			uint16_t length = 0;
			read_be(length);
			str.append(current, length);
			current += length;
		}
		/**
		 * Skip N bytes
		 */
		void skip(std::size_t N)
		{
			current += N;
		}

	  private:
		char *start, *end, *current;
	};
} // namespace util
