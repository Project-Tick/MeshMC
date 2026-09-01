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
#include <QRegularExpression>

class Filter
{
  public:
	virtual ~Filter();
	virtual bool accepts(const QString& value) = 0;
};

class ContainsFilter : public Filter
{
  public:
	ContainsFilter(const QString& pattern);
	virtual ~ContainsFilter();
	bool accepts(const QString& value) override;

  private:
	QString pattern;
};

class ExactFilter : public Filter
{
  public:
	ExactFilter(const QString& pattern);
	virtual ~ExactFilter();
	bool accepts(const QString& value) override;

  private:
	QString pattern;
};

/* ExactFilter, except that a row which has nothing to say on the role is
 * let through instead of being dropped.
 *
 * The version lists are filtered on ParentVersionRole, which Meta derives
 * from a component's hard "equals" requirement on another component. Some
 * components have no such requirement at all - Fabric Loader and Quilt
 * Loader are version-independent, LWJGL is pulled in by Minecraft rather
 * than pinned to it - and for those the role comes back empty. Filtering
 * them with a plain ExactFilter empties the list, which reads to the user
 * as "there are no versions" when the truth is "this one is not tied to a
 * Minecraft version in the first place". */
class ExactIfPresentFilter : public Filter
{
  public:
	ExactIfPresentFilter(const QString& pattern);
	virtual ~ExactIfPresentFilter();
	bool accepts(const QString& value) override;

  private:
	QString pattern;
};

class RegexpFilter : public Filter
{
  public:
	RegexpFilter(const QString& regexp, bool invert);
	virtual ~RegexpFilter();
	bool accepts(const QString& value) override;

  private:
	QRegularExpression pattern;
	bool invert = false;
};
