/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: GPL-3.0-or-later WITH LicenseRef-MeshMC-MMCO-Module-Exception-1.0
 *
 *   MeshMC - A Custom Launcher for Minecraft
 *   Copyright (C) 2026 Project Tick
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version, with the additional permission
 *   described in the MeshMC MMCO Module Exception 1.0.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   You should have received a copy of the MeshMC MMCO Module Exception 1.0
 *   along with this program.  If not, see <https://projecttick.org/licenses/>.
 */

#include "CatPack.h"
#include "Exception.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

// ===================== BasicCatPack =====================

BasicCatPack::BasicCatPack(const QString& id, const QString& name)
	: m_id(id), m_name(name)
{
}

QString BasicCatPack::id()
{
	return m_id;
}

QString BasicCatPack::name()
{
	return m_name;
}

QString BasicCatPack::path()
{
	QDate now = QDate::currentDate();
	int month = now.month();
	int day = now.day();

	// Christmas: Dec 21 - Dec 29
	if ((month == 12 && day >= 21 && day <= 29))
		return QString(":/backgrounds/%1-xmas").arg(m_id);

	// Spooky: Oct 27 - Nov 2
	if ((month == 10 && day >= 27) || (month == 11 && day <= 2))
		return QString(":/backgrounds/%1-spooky").arg(m_id);

	// Birthday: Oct 28 - Nov 5
	if ((month == 10 && day >= 28) || (month == 11 && day <= 5))
		return QString(":/backgrounds/%1-bday").arg(m_id);

	return QString(":/backgrounds/%1").arg(m_id);
}

// ===================== FileCatPack =====================

FileCatPack::FileCatPack(const QFileInfo& fileInfo) : m_fileInfo(fileInfo) {}

QString FileCatPack::id()
{
	return m_fileInfo.baseName();
}

QString FileCatPack::name()
{
	return m_fileInfo.baseName();
}

QString FileCatPack::path()
{
	return m_fileInfo.absoluteFilePath();
}

// ===================== JsonCatPack =====================

JsonCatPack::JsonCatPack(const QFileInfo& manifestInfo)
{
	QFile file(manifestInfo.absoluteFilePath());
	if (!file.open(QFile::ReadOnly))
		throw Exception(QString("Could not open catpack manifest: %1")
							.arg(manifestInfo.absoluteFilePath()));

	QJsonParseError parseError;
	auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
	if (parseError.error != QJsonParseError::NoError)
		throw Exception(QString("catpack.json parse error: %1")
							.arg(parseError.errorString()));

	auto root = doc.object();
	m_id = manifestInfo.dir().dirName();
	m_name = root.value("name").toString(m_id);
	m_defaultPath = QDir(manifestInfo.absolutePath())
						.absoluteFilePath(root.value("default").toString());

	auto variants = root.value("variants").toArray();
	for (const auto& val : variants) {
		auto obj = val.toObject();
		JsonCatPackVariant v;
		v.path = QDir(manifestInfo.absolutePath())
					 .absoluteFilePath(obj.value("path").toString());

		auto startObj = obj.value("startTime").toObject();
		v.startMonth = startObj.value("month").toInt();
		v.startDay = startObj.value("day").toInt();

		auto endObj = obj.value("endTime").toObject();
		v.endMonth = endObj.value("month").toInt();
		v.endDay = endObj.value("day").toInt();

		m_variants.append(v);
	}
}

QString JsonCatPack::id()
{
	return m_id;
}

QString JsonCatPack::name()
{
	return m_name;
}

QString JsonCatPack::path()
{
	QDate now = QDate::currentDate();

	for (const auto& v : m_variants) {
		bool inRange = false;
		if (v.startMonth <= v.endMonth) {
			// Same year range: e.g., Mar 1 - Jun 30
			QDate start(now.year(), v.startMonth, v.startDay);
			QDate end(now.year(), v.endMonth, v.endDay);
			inRange = (now >= start && now <= end);
		} else {
			// Wraps around year boundary: e.g., Dec 20 - Jan 5
			QDate startThisYear(now.year(), v.startMonth, v.startDay);
			QDate endNextYear(now.year() + 1, v.endMonth, v.endDay);
			QDate startLastYear(now.year() - 1, v.startMonth, v.startDay);
			QDate endThisYear(now.year(), v.endMonth, v.endDay);

			inRange = (now >= startThisYear && now <= endNextYear) ||
					  (now >= startLastYear && now <= endThisYear);
		}

		if (inRange)
			return v.path;
	}

	return m_defaultPath;
}
