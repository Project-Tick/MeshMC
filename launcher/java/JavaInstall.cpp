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

#include "JavaInstall.h"
#include <MMCStrings.h>

bool JavaInstall::operator<(const JavaInstall& rhs)
{
	auto archCompare =
		Strings::naturalCompare(arch, rhs.arch, Qt::CaseInsensitive);
	if (archCompare != 0)
		return archCompare < 0;
	if (id < rhs.id) {
		return true;
	}
	if (id > rhs.id) {
		return false;
	}
	return Strings::naturalCompare(path, rhs.path, Qt::CaseInsensitive) < 0;
}

bool JavaInstall::operator==(const JavaInstall& rhs)
{
	return arch == rhs.arch && id == rhs.id && path == rhs.path;
}

bool JavaInstall::operator>(const JavaInstall& rhs)
{
	return (!operator<(rhs)) && (!operator==(rhs));
}
