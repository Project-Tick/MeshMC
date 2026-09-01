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

ExactIfPresentFilter::ExactIfPresentFilter(const QString& pattern)
	: pattern(pattern)
{
}
ExactIfPresentFilter::~ExactIfPresentFilter() {}
bool ExactIfPresentFilter::accepts(const QString& value)
{
	return value.isEmpty() || value == pattern;
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
