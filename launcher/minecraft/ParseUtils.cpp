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

#include <QDateTime>
#include <QString>
#include "ParseUtils.h"
#include <QDebug>
#include <cstdlib>

QDateTime timeFromS3Time(QString str)
{
	return QDateTime::fromString(str, Qt::ISODate);
}

QString timeToS3Time(QDateTime time)
{
	// this all because Qt can't format timestamps right.
	int offsetRaw = time.offsetFromUtc();
	bool negative = offsetRaw < 0;
	int offsetAbs = std::abs(offsetRaw);

	int offsetSeconds = offsetAbs % 60;
	offsetAbs -= offsetSeconds;

	int offsetMinutes = offsetAbs % 3600;
	offsetAbs -= offsetMinutes;
	offsetMinutes /= 60;

	int offsetHours = offsetAbs / 3600;

	QString raw = time.toString("yyyy-MM-ddTHH:mm:ss");
	raw += (negative ? QChar('-') : QChar('+'));
	raw += QString("%1").arg(offsetHours, 2, 10, QChar('0'));
	raw += ":";
	raw += QString("%1").arg(offsetMinutes, 2, 10, QChar('0'));
	return raw;
}
