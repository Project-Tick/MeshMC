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

#pragma once

#include <QString>
#include <QDate>
#include <QFileInfo>
#include <QList>

class CatPack
{
  public:
	virtual ~CatPack() {}
	virtual QString id() = 0;
	virtual QString name() = 0;
	virtual QString path() = 0;
};

class BasicCatPack : public CatPack
{
  public:
	BasicCatPack(const QString& id, const QString& name);

	QString id() override;
	QString name() override;
	QString path() override;

  private:
	QString m_id;
	QString m_name;
};

class FileCatPack : public CatPack
{
  public:
	explicit FileCatPack(const QFileInfo& fileInfo);

	QString id() override;
	QString name() override;
	QString path() override;

  private:
	QFileInfo m_fileInfo;
};

struct JsonCatPackVariant {
	QString path;
	int startMonth;
	int startDay;
	int endMonth;
	int endDay;
};

class JsonCatPack : public CatPack
{
  public:
	explicit JsonCatPack(const QFileInfo& manifestInfo);

	QString id() override;
	QString name() override;
	QString path() override;

  private:
	QString m_id;
	QString m_name;
	QString m_defaultPath;
	QList<JsonCatPackVariant> m_variants;
};
