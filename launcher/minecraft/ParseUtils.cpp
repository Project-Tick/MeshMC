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
