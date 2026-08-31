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

template <typename T> class DefaultVariable
{
  public:
	DefaultVariable(const T& value)
	{
		defaultValue = value;
	}
	DefaultVariable<T>& operator=(const T& value)
	{
		currentValue = value;
		is_default = currentValue == defaultValue;
		is_explicit = true;
		return *this;
	}
	operator const T&() const
	{
		return is_default ? defaultValue : currentValue;
	}
	bool isDefault() const
	{
		return is_default;
	}
	bool isExplicit() const
	{
		return is_explicit;
	}

  private:
	T currentValue;
	T defaultValue;
	bool is_default = true;
	bool is_explicit = false;
};
