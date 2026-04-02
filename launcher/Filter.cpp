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

#include "Filter.h"

Filter::~Filter() {}

ContainsFilter::ContainsFilter(const QString& pattern) : pattern(pattern) {}
ContainsFilter::~ContainsFilter() {}
bool ContainsFilter::accepts(const QString& value)
{
	return value.contains(pattern);
}

ExactFilter::ExactFilter(const QString& pattern) : pattern(pattern) {}
ExactFilter::~ExactFilter() {}
bool ExactFilter::accepts(const QString& value)
{
	return value == pattern;
}

RegexpFilter::RegexpFilter(const QString& regexp, bool invert) : invert(invert)
{
	pattern.setPattern(regexp);
	pattern.optimize();
}
RegexpFilter::~RegexpFilter() {}
bool RegexpFilter::accepts(const QString& value)
{
	auto match = pattern.match(value);
	bool matched = match.hasMatch();
	return invert ? (!matched) : (matched);
}
