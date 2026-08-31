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

// NOTE: apparently the GNU C library pollutes the global namespace with
// these... undef them.
#ifdef major
#undef major
#endif
#ifdef minor
#undef minor
#endif

class JavaVersion
{
	friend class JavaVersionTest;

  public:
	JavaVersion() {};
	JavaVersion(const QString& rhs);

	JavaVersion& operator=(const QString& rhs);

	bool operator<(const JavaVersion& rhs) const;
	bool operator==(const JavaVersion& rhs) const;
	bool operator>(const JavaVersion& rhs) const;

	bool requiresPermGen() const;

	QString toString() const;

	int major() const
	{
		return m_major;
	}
	int minor() const
	{
		return m_minor;
	}
	int security() const
	{
		return m_security;
	}

  private:
	QString m_string;
	int m_major = 0;
	int m_minor = 0;
	int m_security = 0;
	bool m_parseable = false;
	QString m_prerelease;
};
