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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 * Copyright 2013-2021 MultiMC Contributors
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
#include <QList>
#include <QJsonObject>
#include <memory>
#include "OpSys.h"

class Library;
class Rule;

enum RuleAction { Allow, Disallow, Defer };

QList<std::shared_ptr<Rule>>
rulesFromJsonV4(const QJsonObject& objectWithRules);

class Rule
{
  protected:
	RuleAction m_result;
	virtual bool applies(const Library* parent) = 0;

  public:
	Rule(RuleAction result) : m_result(result) {}
	virtual ~Rule() {};
	virtual QJsonObject toJson() = 0;
	RuleAction apply(const Library* parent)
	{
		if (applies(parent))
			return m_result;
		else
			return Defer;
	}
};

class OsRule : public Rule
{
  private:
	// the OS
	OpSys m_system;
	// the OS version regexp
	QString m_version_regexp;

  protected:
	virtual bool applies(const Library*)
	{
		return (m_system == currentSystem);
	}
	OsRule(RuleAction result, OpSys system, QString version_regexp)
		: Rule(result), m_system(system), m_version_regexp(version_regexp)
	{
	}

  public:
	virtual QJsonObject toJson();
	static std::shared_ptr<OsRule> create(RuleAction result, OpSys system,
										  QString version_regexp)
	{
		return std::shared_ptr<OsRule>(
			new OsRule(result, system, version_regexp));
	}
};

class ImplicitRule : public Rule
{
  protected:
	virtual bool applies(const Library*)
	{
		return true;
	}
	ImplicitRule(RuleAction result) : Rule(result) {}

  public:
	virtual QJsonObject toJson();
	static std::shared_ptr<ImplicitRule> create(RuleAction result)
	{
		return std::shared_ptr<ImplicitRule>(new ImplicitRule(result));
	}
};
